
//
// Created by sheverdin on 12/13/23.
// Refactored for readability and safety
//

#include "ar_ping.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>

// Constants
#define PING_PKT_DATA_SIZE      64
#define PING_TIMEOUT_MS         2000
#define PING_DEFAULT_TTL        64
#define PING_MAX_RETRIES        1
#define PING_BUFFER_SIZE (sizeof(struct iphdr) + sizeof(icmp_packet_t))

// ICMP Packet types
#define ICMP_ECHO_REQUEST       8
#define ICMP_ECHO_REPLY         0


// Packet structures
typedef struct {
    struct icmphdr header;
    uint8_t data[PING_PKT_DATA_SIZE];
} icmp_packet_t;

typedef struct {
    uint16_t id;
    uint16_t sequence;
    struct timeval timestamp;
} ping_context_t;

// Helper functions
static uint16_t checksum(void *buffer, size_t length);
static void prepare_packet(icmp_packet_t *packet, ping_context_t *ctx);
static int validate_reply(const icmp_packet_t *reply, const ping_context_t *ctx);
static int64_t time_diff_ms(const struct timeval *start, const struct timeval *end);

// Core ping implementation
ping_error_t ping_host(const char *host)
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0)
    {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return PING_ERR_TEST;
    }

    // Configure socket
    struct timeval tv;
    tv.tv_sec = PING_TIMEOUT_MS;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int ttl = PING_DEFAULT_TTL;
    setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    // Resolve target address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, host, &dest_addr.sin_addr) <= 0)
    {
        close(sock);
        syslog(LOG_ERR, "Invalid address: %s", host);
        return PING_ERR_TEST;
    }

    // Prepare ping context
    ping_context_t ctx =
    {
        .id = (uint16_t)getpid(),
        .sequence = 0
    };
    gettimeofday(&ctx.timestamp, NULL);

    // Create and send packet
    icmp_packet_t packet;
    ping_error_t pingError = PING_OK;
    for (int attempt = 0; attempt < PING_MAX_RETRIES; attempt++)
    {
        pingError = PING_OK;
        ctx.sequence++;
        prepare_packet(&packet, &ctx);
        if (sendto(sock, &packet, sizeof(packet), 0,
                   (struct sockaddr *) &dest_addr, sizeof(dest_addr)) <= 0)
        {
            close(sock);
            syslog(LOG_ERR, "Send failed: %s", strerror(errno));
            pingError = PING_ERR_TEST;
        }

        // Wait for response
        char recv_buffer[PING_BUFFER_SIZE];
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);
        ssize_t bytes_received = recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,  (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_received <= 0)
        {
            syslog(LOG_DEBUG, "No response received");
            pingError = PING_ERR_TEST;
            continue;
        }

        // Пропускаем IP-заголовок (обычно 20 байт)
        struct iphdr *ip_header = (struct iphdr *)recv_buffer;
        size_t ip_header_len = ip_header->ihl * 4;
        if (bytes_received < ip_header_len + sizeof(struct icmphdr))
        {
            syslog(LOG_DEBUG, "Invalid packet size");
            pingError = PING_ERR_TEST;
            continue;
        }

        // Извлекаем ICMP-пакет
        icmp_packet_t *reply = (icmp_packet_t *)(recv_buffer + ip_header_len);
        if (!validate_reply(reply, &ctx))
        {
            syslog(LOG_DEBUG, "Invalid ICMP reply");
            pingError = PING_ERR_TEST;
        }

        if (pingError == PING_OK)
            break;
    }
    close(sock);

    return pingError;
}

// Helper function implementations
static uint16_t checksum(void *buffer, size_t length)
{
    uint32_t sum = 0;
    uint16_t *ptr = buffer;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return (uint16_t)~sum;
}

static void prepare_packet(icmp_packet_t *packet, ping_context_t *ctx)
{
    memset(packet, 0, sizeof(icmp_packet_t));

    // Fill payload with incremental data
    for (size_t i = 0; i < PING_PKT_DATA_SIZE; i++) {
        packet->data[i] = (uint8_t)i;
    }

    // Build ICMP header
    packet->header.type = ICMP_ECHO_REQUEST;
    packet->header.code = 0;
    packet->header.un.echo.id = htons(ctx->id);
    packet->header.un.echo.sequence = htons(ctx->sequence);
    packet->header.checksum = checksum(packet, sizeof(icmp_packet_t));
}

static int validate_reply(const icmp_packet_t *reply, const ping_context_t *ctx)
{
    uint16_t received_checksum = reply->header.checksum;
    icmp_packet_t temp_packet = *reply;
    temp_packet.header.checksum = 0;
    if (checksum(&temp_packet, sizeof(icmp_packet_t)) != received_checksum)
    {
        return 0;
    }

    // Проверка типа, ID и sequence
    return (reply->header.type == ICMP_ECHO_REPLY) &&
           (ntohs(reply->header.un.echo.id) == ctx->id) &&
           (ntohs(reply->header.un.echo.sequence) == ctx->sequence);
}

static int64_t time_diff_ms(const struct timeval *start, const struct timeval *end)
{
    int64_t sec_diff = end->tv_sec - start->tv_sec;
    int64_t usec_diff = end->tv_usec - start->tv_usec;
    return (sec_diff * 1000) + (usec_diff / 1000);
}
