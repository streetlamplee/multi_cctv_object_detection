#include <stdio.h>
#include <stdlib.h>
#include <modbus/modbus.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>

#include "ini.h"
#include "_modbus/modbus_handler.h"

#define INI_FILE "config.ini"

static int g_task_running = FALSE;
static pthread_t g_server_task_thread = 0;
static pthread_t g_config_task_thread = 0;

static modbus_t *ctx = NULL;
static modbus_mapping_t *mb_mapping = NULL;

static void *modbus_handler_server_task(void *arg);
static void *modbus_handler_config_task(void *arg);

static void delay(uint64_t milliseconds);
// 1107 hj aarch64 compile 코드 적용
// static int ini_load_handler(void *config, const char *section, const char *name, const char *value);
// static int ini_save();

void modbus_handler_init()
{
    // 1. Modbus TCP 컨텍스트 생성 (IP = any, 포트 = 502)
    ctx = modbus_new_tcp("0.0.0.0", 502);
    if (ctx == NULL)
    {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return;
    }
    // 1105 hj : modbus slave addr 0 설정
    modbus_set_slave(ctx, 0);
    // 2. 메모리 맵 생성 (예: coils, input bits, holding registers, input registers)
    mb_mapping = modbus_mapping_new(0, 0, 100, 100);
    if (mb_mapping == NULL)
    {
        fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return;
    }

    // 메모리 맵 기본값 설정
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL1_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL2_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL3_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL4_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL5_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL6_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL7_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL8_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL9_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL10_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL11_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL12_ALARM_STATUS] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL1_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL2_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL3_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL4_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL5_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL6_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL7_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL8_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL9_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL10_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL11_ALARM_ID] = 0;
    mb_mapping->tab_registers[MODBUS_IREG_CHANNEL12_ALARM_ID] = 0;
    

    /* // 1107 hj aarch64 compile 코드 적용
    // INI 파일 로딩
    if (access(INI_FILE, F_OK) == 0)
    {
        if (ini_parse(INI_FILE, ini_load_handler, mb_mapping) < 0)
        {
            printf("Can't load '%s'\n", INI_FILE);
            printf("Resetting to default values.\n");
            ini_save();
        }
    }
    else
    {
        ini_save();
    }
    */
}

void modbus_handler_release()
{
    if (mb_mapping != NULL)
    {
        modbus_mapping_free(mb_mapping);
        mb_mapping = NULL;
    }

    if (ctx != NULL)
    {
        modbus_free(ctx);
        ctx = NULL;
    }
}

void modbus_handler_start()
{
    if (g_task_running)
    {
        fprintf(stderr, "Modbus handler is already running.\n");
        return;
    }
    
    g_task_running = TRUE;
    int ret = pthread_create(&g_server_task_thread, NULL, modbus_handler_server_task, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to create modbus handler thread: %s\n", strerror(ret));
        g_task_running = FALSE;
        return;
    }

    ret = pthread_create(&g_config_task_thread, NULL, modbus_handler_config_task, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to create modbus config thread: %s\n", strerror(ret));
        g_task_running = FALSE;
        return;
    }
}

void modbus_handler_stop()
{
    if (!g_task_running)
    {
        fprintf(stderr, "Modbus handler is not running.\n");
        return;
    }
    
    g_task_running = FALSE;
    pthread_join(g_server_task_thread, NULL);
    g_server_task_thread = 0;

    pthread_join(g_config_task_thread, NULL);
    g_config_task_thread = 0;
}

void modbus_handler_set_ireg(int reg, uint16_t value)
{
    if (mb_mapping == NULL)
        return;
    mb_mapping->tab_input_registers[reg] = value;
    printf("Set IREG %d to %d\n", reg, value);
}

void modbus_handler_set_hreg(int reg, uint16_t value)
{
    if (mb_mapping == NULL)
        return;
    mb_mapping->tab_registers[reg] = value;
    printf("Set HREG %d to %d\n", reg, value);
}

uint16_t modbus_handler_get_hreg(int reg)
{
    if (mb_mapping == NULL)
        return 0;
    return mb_mapping->tab_registers[reg];
}

// 1106 hj modbus 적용
uint16_t modbus_handler_get_ireg(int reg)
{
    if (mb_mapping == NULL)
        return 0;
    return mb_mapping->tab_input_registers[reg];
}

void *modbus_handler_server_task(void *arg)
{
    // 3. 소켓 생성 및 대기
    int server_socket = modbus_tcp_listen(ctx, 1);
    if (server_socket == -1)
    {
        perror("modbus_tcp_listen");
        return NULL;
    }

    // 4. Non-blocking 모드 설정
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

    // 5. 클라이언트 연결 처리
    int client_socket;
    while (g_task_running)
    {
        client_socket = modbus_tcp_accept(ctx, &server_socket);
        if (client_socket == -1 || client_socket == -EAGAIN)
        {
            printf("Waiting for client connection...\n");
            delay(500); // 연결이 없으면 잠시 대기
            continue;
        }

        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
        while (g_task_running)
        {
            int rc = modbus_receive(ctx, query);
            // 에러 처리
            if (rc == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    delay(1); // Non-blocking 모드에서 대기
                    continue;
                }
                else if (errno == ECONNRESET)
                {
                    perror("Modbus");
                    break; // 클라이언트가 연결을 끊었음
                }
                else
                {
                    perror("Modbus");
                    break; // 다른 에러 발생
                }
            }

            // 6. 요청에 대한 응답 전송
            modbus_reply(ctx, query, rc, mb_mapping);
        }

        close(client_socket);
    }

    // 7. 자원 해제
    close(server_socket);

    return NULL;
}

void *modbus_handler_config_task(void *arg)
{
    // 주기적으로 INI 파일 저장
    while(g_task_running){        
        // 1107 hj aarch64 compile 코드 적용
        // ini_save();
        delay(1000); // 1초마다 저장
    }
}

void delay(uint64_t milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/* // 1107 hj aarch64 compile 코드 적용
static int ini_load_handler(void *config, const char *section, const char *name, const char *value)
{
    modbus_mapping_t *m = (modbus_mapping_t *)config;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("cam1", "roi_x"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA1_ROI_X] = atoi(value);
    }
    else if (MATCH("cam1", "roi_y"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA1_ROI_Y] = atoi(value);
    }
    else if (MATCH("cam1", "roi_w"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA1_ROI_WIDTH] = atoi(value);
    }
    else if (MATCH("cam1", "roi_h"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA1_ROI_HEIGHT] = atoi(value);
    }
    else if (MATCH("cam1", "conf_thres"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA1_CONFIDENCE_THRESHOLD] = atoi(value);
    }
    else if (MATCH("cam2", "roi_x"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA2_ROI_X] = atoi(value);
    }
    else if (MATCH("cam2", "roi_y"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA2_ROI_Y] = atoi(value);
    }
    else if (MATCH("cam2", "roi_w"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA2_ROI_WIDTH] = atoi(value);
    }
    else if (MATCH("cam2", "roi_h"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA2_ROI_HEIGHT] = atoi(value);
    }
    else if (MATCH("cam2", "conf_thres"))
    {
        m->tab_registers[MODBUS_HREG_CAMERA2_CONFIDENCE_THRESHOLD] = atoi(value);
    }
    else
    {
        fprintf(stderr, "Unknown section/name: [%s] %s = %s\n", section, name, value);
        return 0; // unknown section/name, error
    }
    return 1;
}

int ini_save()
{
    FILE* fp = fopen(INI_FILE, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    // cam1
    fprintf(fp, "[cam1]\n");
    fprintf(fp, "roi_x = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA1_ROI_X]);
    fprintf(fp, "roi_y = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA1_ROI_Y]);
    fprintf(fp, "roi_w = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA1_ROI_WIDTH]);
    fprintf(fp, "roi_h = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA1_ROI_HEIGHT]);
    fprintf(fp, "conf_thres = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA1_CONFIDENCE_THRESHOLD]);
    fprintf(fp, "\n");

    // cam2
    fprintf(fp, "[cam2]\n");
    fprintf(fp, "roi_x = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA2_ROI_X]);
    fprintf(fp, "roi_y = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA2_ROI_Y]);
    fprintf(fp, "roi_w = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA2_ROI_WIDTH]);
    fprintf(fp, "roi_h = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA2_ROI_HEIGHT]);
    fprintf(fp, "conf_thres = %d\n",
            mb_mapping->tab_registers[MODBUS_HREG_CAMERA2_CONFIDENCE_THRESHOLD]);
    fprintf(fp, "\n");

    fclose(fp);
    return 0;
}
*/
