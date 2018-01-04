#ifndef MISC_FUNCTIONS__H
#define MISC_FUNCTIONS__H
#include <string>
#include <netinet/in.h>

int32_t strtoint32(const std::string& str);
int64_t strtoint64(const std::string& str);
std::string inttostr(int i);
std::string hex_decode(const std::string &in);
std::string bintohex(const std::string &in);
void *get_in_addr(struct sockaddr *sa);
std::string bencode_int(int data);
std::string bencode_str(std::string data);
bool private_ipv4(in_addr addr);
bool private_ipv6(in6_addr addr);
#endif
