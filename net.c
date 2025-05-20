#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if _WIN32
#include <WinSock2.h>
#else
#include <sys/select.h>
#endif

#include "timer.h"
#include "bitpack.h"
#include "circbuf.h"
#include "player.h"
#include "net.h"

#define REDUNDANT_INPUTS 1

#define ADDR_FMT "%u.%u.%u.%u:%u"
#define ADDR_LST(addr) (addr)->a,(addr)->b,(addr)->c,(addr)->d,(addr)->port

#define ENABLE_SERVER_LOGGING 0
#define SERVER_LOG_MODE 1  // (0=SIMPLE, 1=VERBOSE)

#if SERVER_PRINT_MODE==1
    #define LOGNV(format, ...) LOGN(format, ##__VA_ARGS__)
#else
    #define LOGNV(format, ...) 0
#endif

#define GAME_ID 0x308B4134

#define PORT 27001

#define MAXIMUM_RTT 1.0f

#define DEFAULT_TIMEOUT 1.0f // seconds
#define PING_PERIOD 3.0f
#define DISCONNECTION_TIMEOUT 7.0f // seconds
#define INPUT_QUEUE_MAX 16
#define MAX_NET_EVENTS 255
#define INPUTS_PER_PACKET 1

typedef struct
{
    int socket;
    uint16_t local_latest_packet_id;
    uint16_t remote_latest_packet_id;
} NodeInfo;

// Info server stores about a client
typedef struct
{
    int client_id;
    Address address;
    ConnectionState state;
    uint16_t remote_latest_packet_id;
    double  time_of_latest_packet;
    uint8_t client_salt[8];
    uint8_t server_salt[8];
    uint8_t xor_salts[8];
    ConnectionRejectionReason last_reject_reason;
    PacketError last_packet_error;
    Packet prior_state_pkt;
    NetPlayerInput net_player_inputs[INPUT_QUEUE_MAX];
    int input_count;
} ClientInfo;

struct
{
    Address address;
    NodeInfo info;
    ClientInfo clients[MAX_CLIENTS];
    NetEvent events[MAX_NET_EVENTS];
    int event_count;
    BitPack bp;
    double start_time;
    int num_clients;
    uint8_t frame_no;
} server = {0};

struct
{
    int id;
    bool received_init_packet;

    Address address;
    NodeInfo info;
    ConnectionState state;
    NetPlayerInput net_player_inputs[INPUT_QUEUE_MAX];
    BitPack bp;

    double time_of_connection;
    double time_of_latest_sent_packet;
    double time_of_last_ping;
    double time_of_last_received_ping;

    Packet state_packets[2]; // for jitter compensation
    int state_packet_count;

    int input_count;
    uint8_t frame_no;

    double rtt;

    uint32_t bytes_received;
    uint32_t bytes_sent;

    uint32_t largest_packet_size_sent;
    uint32_t largest_packet_size_recv;

    uint8_t player_count;
    uint8_t server_salt[8];
    uint8_t client_salt[8];
    uint8_t xor_salts[8];

} client = {0};

// ---

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

// ---

static uint64_t rand64(void)
{
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned) rand();
    }
    return r;
}

static Timer server_timer = {0};

static inline int get_packet_size(Packet* pkt)
{
    return (sizeof(pkt->hdr) + pkt->data_len + sizeof(pkt->data_len));
}

static inline bool is_packet_id_greater(uint16_t id, uint16_t cmp)
{
    return ((id >= cmp) && (id - cmp <= 32768)) ||
           ((id <= cmp) && (cmp - id  > 32768));
}

static char* packet_type_to_str(PacketType type)
{
    switch(type)
    {
        case PACKET_TYPE_INIT: return "INIT";
        case PACKET_TYPE_CONNECT_REQUEST: return "CONNECT REQUEST";
        case PACKET_TYPE_CONNECT_CHALLENGE: return "CONNECT CHALLENGE";
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP: return "CONNECT CHALLENGE RESP";
        case PACKET_TYPE_CONNECT_ACCEPTED: return "CONNECT ACCEPTED";
        case PACKET_TYPE_CONNECT_REJECTED: return "CONNECT REJECTED";
        case PACKET_TYPE_DISCONNECT: return "DISCONNECT";
        case PACKET_TYPE_PING: return "PING";
        case PACKET_TYPE_INPUT: return "INPUT";
        case PACKET_TYPE_SETTINGS: return "SETTINGS";
        case PACKET_TYPE_STATE: return "STATE";
        case PACKET_TYPE_MESSAGE: return "MESSAGE";
        case PACKET_TYPE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static char* connect_reject_reason_to_str(ConnectionRejectionReason reason)
{
    switch(reason)
    {
        case CONNECT_REJECT_REASON_SERVER_FULL: return "SERVER FULL";
        case CONNECT_REJECT_REASON_INVALID_PACKET: return "INVALID PACKET FORMAT";
        case CONNECT_REJECT_REASON_FAILED_CHALLENGE: return "FAILED CHALLENGE";
        default: return "UNKNOWN";
    }
}

static void print_salt(uint8_t* salt)
{
    LOGN("[SALT] %02X %02X %02X %02X %02X %02X %02X %02X",
            salt[0], salt[1], salt[2], salt[3],
            salt[4], salt[5], salt[6], salt[7]
    );
}

static void store_xor_salts(uint8_t* cli_salt, uint8_t* svr_salt, uint8_t* xor_salts)
{
    for(int i = 0; i < 8; ++i)
    {
        xor_salts[i] = (cli_salt[i] ^ svr_salt[i]);
    }
}

static bool is_local_address(Address* addr)
{
    return (addr->a == 127 && addr->b == 0 && addr->c == 0 && addr->d == 1);
}

static bool is_empty_address(Address* addr)
{
    return (addr->a == 0 && addr->b == 0 && addr->c == 0 && addr->d == 0);
}

static void print_address(Address* addr)
{
    LOGN("[ADDR] " ADDR_FMT, ADDR_LST(addr));
}

static bool compare_address(Address* addr1, Address* addr2, bool incl_port)
{
    if(incl_port)
    {
        return (memcmp(addr1, addr2, sizeof(Address)) == 0);
    }

    return (addr1->a == addr2->a && addr1->b == addr2->b && addr1->c == addr2->c && addr1->d == addr2->d);
}


static void print_packet(Packet* pkt, bool full)
{
    LOGN("Game ID:      0x%08x",pkt->hdr.game_id);
    LOGN("Packet ID:    %u",pkt->hdr.id);
    LOGN("Packet Type:  %s (%02X)",packet_type_to_str(pkt->hdr.type),pkt->hdr.type);
    LOGN("Data (%u):", pkt->data_len);

    #define PSIZE 32

    int idx = 0;
    for(;;)
    {
        int size = MIN(PSIZE, (int)pkt->data_len - idx);
        if(size <= 0) break;

        char data[3*PSIZE+5] = {0};
        char byte[4] = {0};

        for(int i = 0; i < size; ++i)
        {
            sprintf(byte,"%02X ",pkt->data[idx+i]);
            memcpy(data+(3*i), byte,3);
        }

        if(!full && pkt->data_len > PSIZE)
        {
            LOGN("%s ...", data);
            return;
        }

        LOGN("%s", data);

        idx += size;
    }

}

static void print_packet_simple(Packet* pkt, const char* hdr)
{
    LOGN("[%s][ID: %u] %s (%u B)",hdr, pkt->hdr.id, packet_type_to_str(pkt->hdr.type), pkt->data_len);
}

static bool has_data_waiting(int socket)
{

    fd_set readfds;

    //clear the socket set
    FD_ZERO(&readfds);

    //add client socket to set
    FD_SET(socket, &readfds);

    int activity;

    struct timeval tv = {0};
    activity = select(socket + 1 , &readfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        perror("select error");
        return false;
    }

    bool has_data = FD_ISSET(socket , &readfds);
    return has_data;
}

static int net_send(NodeInfo* node_info, Address* to, Packet* pkt, int count)
{

    // static bool f = false;
    // if(!f)
    // {
    //     f = true;
    //     node_info->local_latest_packet_id = 32500;
    // }

    int pkt_len = get_packet_size(pkt);
    int sent_bytes = 0;

    for(int i = 0; i < count; ++i)
        sent_bytes += socket_sendto(node_info->socket, to, (uint8_t*)pkt, pkt_len);

#if ENABLE_SERVER_LOGGING
#if SERVER_LOG_MODE==0
    print_packet_simple(pkt,"SEND");
#elif SERVER_LOG_MODE==1
    LOGN("[SENT] Packet %d (%u B)",pkt->hdr.id,sent_bytes);
    print_packet(pkt, false);
#endif
#endif

    node_info->local_latest_packet_id++;

    if(node_info == &client.info)
    {
        client.bytes_sent += sent_bytes;
        if(sent_bytes > client.largest_packet_size_sent)
        {
            client.largest_packet_size_sent = sent_bytes;
        }
    }

    return sent_bytes;
}

static int net_recv(NodeInfo* node_info, Address* from, Packet* pkt)
{
    int recv_bytes = socket_recvfrom(node_info->socket, from, (uint8_t*)pkt);

#if ENABLE_SERVER_LOGGING
#if SERVER_LOG_MODE==0
    print_packet_simple(pkt,"RECV");
#else
    LOGN("[RECV] Packet %d (%u B)",pkt->hdr.id,recv_bytes);
    print_address(from);
    print_packet(pkt, false);
#endif
#endif

    if(node_info == &client.info)
    {
        client.bytes_received += recv_bytes;
        if(recv_bytes > client.largest_packet_size_recv)
        {
            client.largest_packet_size_recv = recv_bytes;
        }
    }

    return recv_bytes;
}

static bool validate_packet_format(Packet* pkt)
{
    if(pkt->hdr.game_id != GAME_ID)
    {
        LOGN("Game ID of packet doesn't match, %08X != %08X",pkt->hdr.game_id, GAME_ID);
        return false;
    }

    if(pkt->hdr.type < 0 || pkt->hdr.type >= PACKET_TYPE_MAX)
    {
        LOGN("Invalid Packet Type: %d", pkt->hdr.type);
        return false;
    }

    return true;
}

static bool authenticate_client(Packet* pkt, ClientInfo* cli)
{
    bool valid = true;

    switch(pkt->hdr.type)
    {
        case PACKET_TYPE_CONNECT_REQUEST:
            valid &= (pkt->data_len == 1024); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->client_salt, 8) == 0);
            break;
        case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
            valid &= (pkt->data_len == 1024); // must be padded out to 1024
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
           break;
        default:
            valid &= (memcmp(&pkt->data[0],cli->xor_salts, 8) == 0);
            break;
    }

    return valid;
}

static int server_get_client(Address* addr, ClientInfo** cli)
{
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        bool addr_check = compare_address(&server.clients[i].address, addr, true);
        bool conn = server.clients[i].state != DISCONNECTED;

        if(addr_check && conn)
        {
            *cli = &server.clients[i];
            return i;
        }
    }

    return -1;
}

// 0: unable to assign new client
// 1: assigned new client
// 2: assigned new client and assigned them to their previous spot
static int server_assign_new_client(Address* addr, ClientInfo** cli, char* name)
{
    LOGN("server_assign_new_client()");
    print_address(addr);

    // new client
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state == DISCONNECTED)
        {
            *cli = &server.clients[i];
            (*cli)->client_id = i;
            LOGN("Assigning new client: %d", (*cli)->client_id);
            return 1;
        }
    }

    LOGN("Server is full and can't accept new clients.");
    return 0;
}

static void update_server_num_clients()
{
    int num_clients = 0;
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(server.clients[i].state != DISCONNECTED)
        {
            num_clients++;
        }
    }
    server.num_clients = num_clients;

    if(server.num_clients == 0)
    {
        LOGN("Everyone left!");
    }
}

static void remove_client(ClientInfo* cli)
{
    LOGN("Remove client: %d", cli->client_id);
    cli->state = DISCONNECTED;
    cli->remote_latest_packet_id = 0;
    player_set_active(&players[cli->client_id],false);
    memset(cli,0, sizeof(ClientInfo));

    update_server_num_clients();
}

static void server_send(PacketType type, ClientInfo* cli)
{
    Packet pkt = {
        .hdr.game_id = GAME_ID,
        .hdr.id = server.info.local_latest_packet_id,
        .hdr.ack = cli->remote_latest_packet_id,
        .hdr.frame_no = server.frame_no,
        .hdr.type = type
    };

    LOGNV("%s() : %s", __func__, packet_type_to_str(type));

    switch(type)
    {
        case PACKET_TYPE_INIT:
        {
        } break;

        case PACKET_TYPE_CONNECT_CHALLENGE:
        {
            uint64_t salt = rand64();
            memcpy(cli->server_salt, (uint8_t*)&salt,8);

            // store xor salts
            store_xor_salts(cli->client_salt, cli->server_salt, cli->xor_salts);
            print_salt(cli->xor_salts);

            pack_bytes(&pkt, cli->client_salt, 8);
            pack_bytes(&pkt, cli->server_salt, 8);

            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_CONNECT_ACCEPTED:
        {
            cli->state = CONNECTED;
            pack_u8(&pkt, (uint8_t)cli->client_id);
            net_send(&server.info,&cli->address,&pkt, 1);

            refresh_visible_room_gun_list();
            server_send_message(TO_ALL, FROM_SERVER, "client added %u", cli->client_id);
        } break;

        case PACKET_TYPE_CONNECT_REJECTED:
        {
            pack_u8(&pkt, (uint8_t)cli->last_reject_reason);
            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_PING:
            pkt.data_len = 0;
            net_send(&server.info,&cli->address,&pkt, 1);
            break;

        case PACKET_TYPE_SETTINGS:
        {
        } break;

        case PACKET_TYPE_STATE:
        {

        } break;

        case PACKET_TYPE_ERROR:
        {
            pack_u8(&pkt, (uint8_t)cli->last_packet_error);
            net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        case PACKET_TYPE_DISCONNECT:
        {
            cli->state = DISCONNECTED;
            pkt.data_len = 0;
            // redundantly send so packet is guaranteed to get through
            for(int i = 0; i < 3; ++i)
                net_send(&server.info,&cli->address,&pkt, 1);
        } break;

        default:
            break;
    }
}

static void server_simulate(double dt)
{
    int player_count = 0;

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        ClientInfo* cli = &server.clients[i];
        if(cli->state != CONNECTED)
            continue;

        player_count++;
        Player* p = &players[cli->client_id];

        if(cli->input_count == 0)
        {
            player_update(p,dt, false, 0);
        }
        else
        {
            for(int i = 0; i < cli->input_count; ++i)
            {
                // apply input to player
                for(int j = 0; j < PLAYER_ACTION_MAX; ++j)
                {
                    bool key_state = (cli->net_player_inputs[i].keys & ((uint32_t)1<<j)) != 0;
                    p->actions[j].state = key_state;
                }

                player_update(p,cli->net_player_inputs[i].delta_t, false, 0);
            }

            cli->input_count = 0;
        }
    }

    server.frame_no++;
    if(server.frame_no > 255)
        server.frame_no = 0;
}

int net_server_start()
{
    LOGN("%s()", __func__);

    // init
    socket_initialize();

    memset(server.clients, 0, sizeof(ClientInfo)*MAX_CLIENTS);
    server.num_clients = 0;

    int sock;

    // set timers
    timer_set_fps(&server_timer,TICK_RATE);
    timer_begin(&server_timer);

    LOGN("Creating socket.");
    socket_create(&sock);

    LOGN("Binding socket %u to any local ip on port %u.", sock, PORT);
    socket_bind(sock, NULL, PORT);
    server.info.socket = sock;

    bitpack_create(&server.bp, BITPACK_SIZE);

    LOGN("Server Started with tick rate %f.", TICK_RATE);

    double t0=timer_get_time();
    double t1=0.0;
    double accum = 0.0;

    server.start_time = t0;

    double t0_g=timer_get_time();
    double t1_g=0.0;
    double accum_g = 0.0;

    const double dt = 1.0/TICK_RATE;

    for(;;)
    {
        // handle connections, receive inputs
        for(;;)
        {
            // Read all pending packets
            bool data_waiting = has_data_waiting(server.info.socket);
            if(!data_waiting)
            {
                break;
            }

            Address from = {0};
            Packet recv_pkt = {0};
            int offset = 0;

            int bytes_received = net_recv(&server.info, &from, &recv_pkt);

            if(!validate_packet_format(&recv_pkt))
            {
                LOGN("Invalid packet format!");
                timer_delay_us(1000); // delay 1ms
                continue;
            }

            ClientInfo* cli = NULL;

            if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_REQUEST)
            {
                if(recv_pkt.data_len != 1024)
                {
                    LOGN("Packet length doesn't equal %d",1024);
                    break;
                }

                uint8_t salt[8] = {0};
                unpack_bytes(&recv_pkt, salt, 8, &offset);

                char name[PLAYER_NAME_MAX+1] = {0};
                uint8_t namelen = unpack_string(&recv_pkt, name, PLAYER_NAME_MAX, &offset);
                if(namelen == 0) printf("namelen is 0!\n");

                int ret = server_assign_new_client(&from, &cli, name);

                if(ret > 0)
                {
                    cli->state = SENDING_CONNECTION_REQUEST;
                    memcpy(&cli->address,&from,sizeof(Address));
                    update_server_num_clients();

                    LOGN("Welcome New Client! (%d/%d)", server.num_clients, MAX_CLIENTS);
                    print_address(&cli->address);

                    if(ret == 1)
                    {
                        player_reset(&players[cli->client_id]);
                    }

                    // store salt
                    memcpy(cli->client_salt, salt, 8);
                    server_send(PACKET_TYPE_CONNECT_CHALLENGE, cli);
                }
                else
                {
                    LOGNV("Creating temporary client");
                    // create a temporary ClientInfo so we can send a reject packet back
                    ClientInfo tmp_cli = {0};
                    memcpy(&tmp_cli.address,&from,sizeof(Address));

                    tmp_cli.last_reject_reason = CONNECT_REJECT_REASON_SERVER_FULL;
                    server_send(PACKET_TYPE_CONNECT_REJECTED, &tmp_cli);
                    break;
                }
            }
            else
            {

               int client_id = server_get_client(&from, &cli);
               if(client_id == -1) break;

                // existing client
                bool auth = authenticate_client(&recv_pkt,cli);
                offset = 8;

                if(!auth)
                {
                    LOGN("Client Failed authentication");

                    if(recv_pkt.hdr.type == PACKET_TYPE_CONNECT_CHALLENGE_RESP)
                    {
                        cli->last_reject_reason = CONNECT_REJECT_REASON_FAILED_CHALLENGE;
                        server_send(PACKET_TYPE_CONNECT_REJECTED,cli);
                        remove_client(cli);
                    }
                    break;
                }

                bool is_latest = is_packet_id_greater(recv_pkt.hdr.id, cli->remote_latest_packet_id);
                if(!is_latest)
                {
                    LOGN("Not latest packet from client. Ignoring...");
                    break;
                }

                cli->remote_latest_packet_id = recv_pkt.hdr.id;
                cli->time_of_latest_packet = timer_get_time();

                LOGNV("%s() : %s", __func__, packet_type_to_str(recv_pkt.hdr.type));

                switch(recv_pkt.hdr.type)
                {

                    case PACKET_TYPE_CONNECT_CHALLENGE_RESP:
                    {
                        cli->state = SENDING_CHALLENGE_RESPONSE;
                        LOGI("Accept client: %d", cli->client_id);
                        player_set_active(&players[cli->client_id],true);

                        server_send(PACKET_TYPE_CONNECT_ACCEPTED,cli);
                        server_send(PACKET_TYPE_INIT, cli);
                        server_send(PACKET_TYPE_STATE,cli);

                    } break;

                    case PACKET_TYPE_INPUT:
                    {
                        uint8_t _input_count = unpack_u8(&recv_pkt, &offset);
                        for(int i = 0; i < _input_count; ++i)
                        {
                            // get input, copy into array
                            unpack_bytes(&recv_pkt, (uint8_t*)&cli->net_player_inputs[cli->input_count++], sizeof(NetPlayerInput), &offset);
                        }
                    } break;

                    case PACKET_TYPE_MESSAGE:
                    {
                    } break;

                    case PACKET_TYPE_SETTINGS:
                    {
                    } break;

                    case PACKET_TYPE_PING:
                    {
                        server_send(PACKET_TYPE_PING, cli);
                    } break;

                    case PACKET_TYPE_DISCONNECT:
                    {
                        remove_client(cli);
                    } break;

                    default:
                    break;
                }
            }
        }

        t1_g = timer_get_time();
        double elapsed_time_g = t1_g - t0_g;
        t0_g = t1_g;

        accum_g += elapsed_time_g;
        double _dt = 1.0/TARGET_FPS;
        while(accum_g >= _dt)
        {
            g_timer += _dt;
            server_simulate(_dt);
            accum_g -= _dt;
        }

        t1 = timer_get_time();
        double elapsed_time = t1 - t0;
        t0 = t1;

        accum += elapsed_time;

        if(accum >= dt)
        {
            // send state packet to all clients
            if(server.num_clients > 0)
            {
                // disconnect any client that hasn't sent a packet in DISCONNECTION_TIMEOUT
                for(int i = 0; i < MAX_CLIENTS; ++i)
                {
                    ClientInfo* cli = &server.clients[i];

                    if(cli == NULL) continue;
                    if(cli->state == DISCONNECTED) continue;

                    if(cli->time_of_latest_packet > 0)
                    {
                        double time_elapsed = timer_get_time() - cli->time_of_latest_packet;

                        if(time_elapsed >= DISCONNECTION_TIMEOUT)
                        {
                            LOGN("Client timed out. Elapsed time: %f", time_elapsed);

                            // disconnect client
                            server_send(PACKET_TYPE_DISCONNECT,cli);
                            remove_client(cli);
                            continue;
                        }
                    }

                    // send world state to connected clients...
                    server_send(PACKET_TYPE_STATE,cli);
                }

                // clear out any queued events
                server.event_count = 0;
            }
            accum = 0.0;
        }

        timer_delay_us(1000); // 1ms delay to prevent cpu % from going nuts
    }
}
