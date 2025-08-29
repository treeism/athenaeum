#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define IP_4_CLASS_A (1 << 0)
#define IP_4_CLASS_B (1 << 1)
#define IP_4_CLASS_C (1 << 2)
#define IP_4_CLASS_D (1 << 3)
#define IP_4_CLASS_E (1 << 4)

#ifdef _WIN32
typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
#endif

// Simple list implementation for demonstration
typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} list;

list* list_new() {
    list *l = malloc(sizeof(list));
    l->data = malloc(sizeof(void*) * 10);
    l->size = 0;
    l->capacity = 10;
    return l;
}

void list_add(list *l, void *item) {
    if (l->size >= l->capacity) {
        l->capacity *= 2;
        l->data = realloc(l->data, sizeof(void*) * l->capacity);
    }
    l->data[l->size++] = item;
}

void* list_get(list *l, size_t index) {
    if (index < l->size) return l->data[index];
    return NULL;
}

size_t list_size(list *l) {
    return l->size;
}

void list_delete(list *l) {
    free(l->data);
    free(l);
}

void list_free(list *l) {
    for(size_t i = 0; i < list_size(l); i++) {
        void *c = list_get(l, i);
        free(c);
    }
    list_delete(l);
}

typedef struct {
    u_int8_t o[4];
} ip_4;

typedef struct {
    u_int16_t h[8];
} ip_6;

void ip_4_print(ip_4 ip) {
    for(u_int8_t i = 0; i < 4; i++) {
        printf("%hhu", ip.o[i]);
        if(i != 3)
            printf(".");
    }
}

void ip_4_println(ip_4 ip) {
    ip_4_print(ip);
    printf("\n");
}

u_int8_t ip_4_get_class(ip_4 ip) {
    if(ip.o[0] >= 240)
        return IP_4_CLASS_E;
    if(ip.o[0] >= 224)
        return IP_4_CLASS_D;
    if(ip.o[0] >= 192)
        return IP_4_CLASS_C;
    if(ip.o[0] >= 128)
        return IP_4_CLASS_B;
    return IP_4_CLASS_A;
}

u_int32_t ip_4_to_value(ip_4 ip) {
    u_int32_t res = 0;
    for (u_int8_t i = 0; i < 4; i++) {
        res <<= 8;
        res |= ip.o[i];
    }
    return res;
}

ip_4 value_to_ip_4(u_int32_t value) {
    ip_4 ip;
    for (int i = 3; i >= 0; i--) {
        ip.o[i] = value & 0xFF;
        value >>= 8;
    }
    return ip;
}

u_int32_t sm_address_to_slash(ip_4 value) {
    u_int32_t ip_value = ip_4_to_value(value);
    u_int32_t count = 0;
    
    // Count consecutive 1s from the left
    for (u_int32_t i = 31; i < 32; i--) {  // Using unsigned wraparound
        if (ip_value & (1U << i)) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

ip_4 sm_slash_to_address(u_int32_t value) {
    if (value > 32) {
        return (ip_4) {0, 0, 0, 0};
    }
    
    u_int32_t mask = 0;
    if (value > 0) {
        mask = (0xFFFFFFFFU << (32 - value));
    }
    
    return value_to_ip_4(mask);
}

typedef struct {
    ip_4 address;
    ip_4 subnet_mask;
} ip_d;

ip_4 ip_network(ip_4 ip) {
    u_int8_t ni = 0;
    switch(ip_4_get_class(ip)) {
        case IP_4_CLASS_A:
            ni = 1;
            break;
        case IP_4_CLASS_B:
            ni = 2;
            break;
        case IP_4_CLASS_C:
            ni = 3;
            break;
        default:
            return (ip_4) {0,0,0,0};
    }
    
    ip_4 result = ip;
    for (int i = ni; i < 4; i++)
        result.o[i] = 0;
    return result;
}

ip_4 ip_broadcast(ip_4 ip) {
    u_int8_t ni = 0;
    switch(ip_4_get_class(ip)) {
        case IP_4_CLASS_A:
            ni = 1;
            break;
        case IP_4_CLASS_B:
            ni = 2;
            break;
        case IP_4_CLASS_C:
            ni = 3;
            break;
        default:
            return (ip_4) {0,0,0,0};
    }
    
    ip_4 result = ip;
    for (int i = ni; i < 4; i++)
        result.o[i] = 255;
    return result;
}

size_t calc_sizeof_qt(size_t qt) {
    if (qt == 0)
        return 0;
    return (size_t)ceil(log2(qt));
}

size_t calc_qtof_size(size_t size) {
    if (size == 0)
        return 0;
    return (size_t)pow(2, size);
}

ip_4 calc_subnet_mask(ip_4 ip, size_t qt_subnet) {
    u_int8_t class_bits = 0;
    switch(ip_4_get_class(ip)) {
        case IP_4_CLASS_A:
            class_bits = 8;
            break;
        case IP_4_CLASS_B:
            class_bits = 16;
            break;
        case IP_4_CLASS_C:
            class_bits = 24;
            break;
        default:
            return (ip_4) {0,0,0,0};
    }
    
    size_t subnet_bits = calc_sizeof_qt(qt_subnet);
    u_int32_t total_mask_bits = class_bits + subnet_bits;
    
    if (total_mask_bits > 32) {
        total_mask_bits = 32;
    }
    
    return sm_slash_to_address(total_mask_bits);
}

size_t calc_subnet_qt(ip_4 ip, ip_4 subnet_mask) {
    u_int8_t class_bits = 0;
    switch (ip_4_get_class(ip)) {
        case IP_4_CLASS_A:
            class_bits = 8;
            break;
        case IP_4_CLASS_B:
            class_bits = 16;
            break;
        case IP_4_CLASS_C:
            class_bits = 24;
            break;
        default:
            return 0;
    }
    
    u_int32_t mask_bits = sm_address_to_slash(subnet_mask);
    if (mask_bits <= class_bits) {
        return 1;  // No subnetting
    }
    
    u_int32_t subnet_bits = mask_bits - class_bits;
    return (size_t)pow(2, subnet_bits);
}

size_t calc_host_qt(ip_4 ip, ip_4 subnet_mask) {
    u_int32_t mask_bits = sm_address_to_slash(subnet_mask);
    u_int32_t host_bits = 32 - mask_bits;
    
    if (host_bits <= 1) {
        return 0;  // Not enough bits for hosts
    }
    
    return ((size_t)pow(2, host_bits)) - 2;  // Subtract network and broadcast
}

int same_net_subnet(ip_4 x, ip_4 y, ip_4 subnet_mask) {
    u_int32_t ip1 = ip_4_to_value(x);
    u_int32_t ip2 = ip_4_to_value(y);
    u_int32_t mask = ip_4_to_value(subnet_mask);

    return (ip1 & mask) == (ip2 & mask);
}

// CIDR function that calculates network and populates the list
ip_4 ip_cidr(list **output, size_t nhosts, ip_4 base) {
    if (nhosts == 0) {
        *output = list_new();
        return (ip_4) {0, 0, 0, 0};
    }
    
    // Calculate required host bits (add 2 for network and broadcast)
    size_t host_bits = calc_sizeof_qt(nhosts + 2);
    u_int32_t mask_bits = 32 - host_bits;
    
    // Create subnet mask
    ip_4 netmask = sm_slash_to_address(mask_bits);
    
    // Calculate network address
    u_int32_t base_value = ip_4_to_value(base);
    u_int32_t mask_value = ip_4_to_value(netmask);
    u_int32_t network_value = base_value & mask_value;
    
    // Create output list and populate with usable host addresses
    *output = list_new();
    
    for (size_t i = 1; i <= nhosts && i < ((1UL << host_bits) - 1); i++) {
        ip_4 *host_ip = malloc(sizeof(ip_4));
        *host_ip = value_to_ip_4(network_value + i);
        list_add(*output, host_ip);
    }
    
    return netmask;
}

// Utility function to print network information
void print_network_info(ip_4 ip, ip_4 subnet_mask) {
    printf("IP Address: ");
    ip_4_println(ip);
    printf("Subnet Mask: ");
    ip_4_println(subnet_mask);
    printf("CIDR: /%u\n", sm_address_to_slash(subnet_mask));
    printf("Network: ");
    ip_4_println(value_to_ip_4(ip_4_to_value(ip) & ip_4_to_value(subnet_mask)));
    printf("Broadcast: ");
    u_int32_t network = ip_4_to_value(ip) & ip_4_to_value(subnet_mask);
    u_int32_t host_bits = 32 - sm_address_to_slash(subnet_mask);
    u_int32_t broadcast = network | ((1UL << host_bits) - 1);
    ip_4_println(value_to_ip_4(broadcast));
    printf("Available Hosts: %lu\n", calc_host_qt(ip, subnet_mask));
    printf("Subnets: %lu\n", calc_subnet_qt(ip, subnet_mask));
}

int main(void) {
    ip_4 ip = {192,168,1,0};
    ip_4 ip_max = {255,255,255,255};
    ip_4 test = {130,10,67,160};
    ip_4 t_ip =  {150,169,3,8};
    ip_4 t_ip2 = {150,169,3,2};
    ip_4 t_sm =  {255,255,255,0};

    printf("=== IP Address Classification ===\n");
    ip_4_print(ip);
    if(ip_4_get_class(ip) == IP_4_CLASS_C)
        printf(" - Class C\n");

    printf("\n=== CIDR Conversion ===\n");
    printf("255.255.255.255 = /%u\n", sm_address_to_slash(ip_max));
    printf("/30 = ");
    ip_4_println(sm_slash_to_address(30));

    printf("\n=== Subnet Calculations ===\n");
    printf("Subnet mask for 7 subnets on ");
    ip_4_print(test);
    printf(": ");
    ip_4_println(calc_subnet_mask(test, 7));

    printf("Network ");
    ip_4_print(t_ip);
    printf(" with mask ");
    ip_4_print(t_sm);
    printf(" has %lu subnets & %lu hosts\n", calc_subnet_qt(t_ip, t_sm), calc_host_qt(t_ip, t_sm));

    printf("Same network check: %s\n", same_net_subnet(t_ip, t_ip2, t_sm) ? "YES" : "NO");

    printf("\n=== CIDR Allocation ===\n");
    list *host_list = NULL;
    ip_4 base = {200,45,8,0};
    size_t nhosts = 10;

    printf("Allocating %lu hosts starting from ", nhosts);
    ip_4_println(base);
    
    ip_4 allocated_mask = ip_cidr(&host_list, nhosts, base);
    printf("Allocated subnet mask: ");
    ip_4_println(allocated_mask);
    
    if (host_list) {
        printf("First few allocated addresses:\n");
        for (size_t i = 0; i < list_size(host_list) && i < 5; i++) {
            ip_4 *addr = (ip_4*)list_get(host_list, i);
            printf("  ");
            ip_4_println(*addr);
        }
        if (list_size(host_list) > 5) {
            printf("  ... and %lu more\n", list_size(host_list) - 5);
        }
        list_free(host_list);
    }

    printf("\n=== Network Information ===\n");
    print_network_info(t_ip, t_sm);

    return 0;
}
