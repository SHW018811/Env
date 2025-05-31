// can_receiver_sim.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <libwebsockets.h>
#include <jansson.h> // for JSON parsing

struct lws_context *ws_context = NULL;
struct lws *ws_client = NULL;
pthread_t ws_thread;

int main(void) {
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // 1) CAN_RAW 소켓 생성
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket 생성 실패");
        return 1;
    }

    // 2) 인터페이스 이름 지정 (vcan0)
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl: 인터페이스 인덱스 가져오기 실패");
        close(sock);
        return 1;
    }

    // 3) 바인딩
    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind 실패");
        close(sock);
        return 1;
    }

    printf("=== vcan0 수신기 실행됨 ===\n");
    printf("이제 'bms_simulator vcan0'가 보낸 CAN 프레임을 여기서 볼 수 있습니다.\n\n");

    // 5) Create a separate thread to handle WebSocket client connection
    void *ws_client_thread(void *arg);
    if (pthread_create(&ws_thread, NULL, ws_client_thread, NULL) != 0) {
        perror("WebSocket thread creation failed");
        // continue without WebSocket functionality
    }

    // 4) 무한 루프: read()로 들어오는 CAN 프레임을 계속 수신
    while (1) {
        int nbytes = read(sock, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("CAN read 오류");
            break;
        }
        if (nbytes < sizeof(struct can_frame)) {
            // 프레임을 전부 못 받은 경우(드물게 발생)
            fprintf(stderr, "짧은 바이트 읽음: %d\n", nbytes);
            continue;
        }

        // 프레임 출력 예시
        printf("수신 ID=0x%03X  DLC=%d  데이터=[", frame.can_id, frame.can_dlc);
        for (int i = 0; i < frame.can_dlc; i++) {
            printf(" %02X", frame.data[i]);
        }
        printf(" ]\n");
    }

    close(sock);
    // Wait for WebSocket client thread to finish (e.g., on program termination)
    pthread_join(ws_thread, NULL);
    return 0;
}


void *ws_client_thread(void *arg) {
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    const char *protocols[] = { "lws-minimal-client", NULL };

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = (const struct lws_protocols *)protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    ws_context = lws_create_context(&info);
    if (!ws_context) {
        fprintf(stderr, "Failed to create WebSocket context\n");
        return NULL;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = ws_context;
    ccinfo.address = "localhost";
    ccinfo.port = 12261;
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(ws_context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0];

    ws_client = lws_client_connect_via_info(&ccinfo);
    if (!ws_client) {
        fprintf(stderr, "WebSocket connection failed\n");
        lws_context_destroy(ws_context);
        return NULL;
    }

    while (1) {
        lws_service(ws_context, 100);

        // Check if a message has been received
        unsigned char buf[512];
        int n = lws_receive(ws_client, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0'; // null-terminate JSON string
            // Parse JSON and extract "type" and "act"
            json_error_t err;
            json_t *root = json_loads((const char *)buf, 0, &err);
            if (root) {
                const char *type = json_string_value(json_object_get(root, "type"));
                const char *act = json_string_value(json_object_get(root, "act"));
                if (type && act && strcmp(type, "CMD") == 0) {
                    // Determine CAN ID and data based on act
                    struct can_frame frame;
                    frame.can_id = 0x010;
                    frame.can_dlc = 1;
                    if (strcmp(act, "STOP_CHARGE") == 0) {
                        frame.data[0] = 0x00;
                    } else if (strcmp(act, "START_CHARGE") == 0) {
                        frame.data[0] = 0x01;
                    } else {
                        // unknown command, ignore
                        json_decref(root);
                        continue;
                    }
                    // Open CAN socket to send
                    int tx_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
                    if (tx_sock >= 0) {
                        struct ifreq ifr;
                        struct sockaddr_can addr;
                        strncpy(ifr.ifr_name, "vcan0", IFNAMSIZ - 1);
                        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
                        if (ioctl(tx_sock, SIOCGIFINDEX, &ifr) >= 0) {
                            addr.can_family = AF_CAN;
                            addr.can_ifindex = ifr.ifr_ifindex;
                            if (bind(tx_sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                                write(tx_sock, &frame, sizeof(struct can_frame));
                            }
                        }
                        close(tx_sock);
                    }
                }
                json_decref(root);
            }
        }
    }

    lws_context_destroy(ws_context);
    return NULL;
}