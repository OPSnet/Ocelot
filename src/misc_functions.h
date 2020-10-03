#ifndef MISC_FUNCTIONS__H
#define MISC_FUNCTIONS__H
#include <string>
#include <netdb.h>

int32_t strtoint32(const std::string& str);
int64_t strtoint64(const std::string& str);
std::string inttostr(int i);
std::string hex_decode(const std::string &in);
std::string bintohex(const std::string &in);
bool ipv4_is_public(in_addr addr);
bool ipv6_is_public(in6_addr addr);
std::string bencode_str(std::string data);
std::string bencode_int(int data);
#endif
