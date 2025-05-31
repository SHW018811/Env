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
    return 0;
}