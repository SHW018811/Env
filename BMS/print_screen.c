#include "all_headers.h"

#define BAR_WIDTH 50
#define LOAD_TIME 100

pthread_mutex_t lock;

void print_battery_bar(int soc){
    printf("\x1b[1J");
    int bar_length = (soc * BAR_WIDTH) / 100;
    int i;
    printf("\x1b[HBattery: [");
    for (i = 0; i < bar_length; i++) {
        printf("=");
    }
    for (; i < BAR_WIDTH; i++) {
        printf(" ");
    }
    printf("] %d%%", soc);
    fflush(stdout);
}

void *print_screen_thread(void *arg) {
    int i;
    
    printf("Loading: [");  // 시작 텍스트
    for (i = 0; i < BAR_WIDTH; i++) {
        printf(" ");  // 초기 빈 공간
    }
    printf("] 0%%");  // 퍼센트 표시
    fflush(stdout);  // 즉시 출력

    for (i = 0; i <= BAR_WIDTH; i++) {
        usleep(LOAD_TIME * 1000);  // 지연 (ms 단위 변환)
        printf("\rLoading: [");  // 캐리지 리턴으로 줄 덮어쓰기
        int j;
        for (j = 0; j < i; j++) {
            printf("=");  // 채워진 부분
        }
        for (; j < BAR_WIDTH; j++) {
            printf(" ");  // 남은 빈 공간
        }
        printf("] %03d%%", (i * 100) / BAR_WIDTH);  // 진행률 표시
        fflush(stdout);
        if (i == BAR_WIDTH) i = 0;
    }

}

// int main() {
//     pthread_t tid1;

//     pthread_mutex_init(&lock, NULL);

//     pthread_create(&tid1, NULL, print_screen_thread, NULL);

//     pthread_join(tid1, NULL);

//     pthread_mutex_destroy(&lock);
// }

int main() {
    int8_t soc;

    while(1) {
        scanf("%d", &soc);
        if (9999 == soc) break;
        print_battery_bar(soc);
    }
    return 0;
}