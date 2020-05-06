#ifndef MISC_FUNCTIONS__H
#define MISC_FUNCTIONS__H
#include <string>
#include <array>

int32_t strtoint32(const std::string& str);
int64_t strtoint64(const std::string& str);
std::string inttostr(int i);

inline std::uint8_t hexchar_to_bin(char in) {
	auto out = static_cast<std::uint8_t>(in);
	if (in >= 'a' && in <= 'f') {
		return out - 'a' + 10;
	} else if (in >= 'A' && in <= 'F') {
		return out - 'A' + 10;
	} else if (in >= '0' && in <= '9') {
		return out - '0';
	} else {
		return '0';
	}
}

template<typename Oiter, typename F>
inline bool hex_decode_impl(const std::string& in, Oiter out, F out_is_end)
{
	unsigned int i;
	for (i = 0; i < in.length() && !out_is_end(out); i++, out++) {
		unsigned char x = '0';
		if (in[i] == '%' && (i + 2) < in.length()) {
			x = (hexchar_to_bin(in[i + 1]) << 4) | hexchar_to_bin(in[i + 2]);
			i += 2;
		} else {
			x = in[i];
		}
		*out = x;
	}
	return (i == in.length());
}

template<std::size_t N>
inline bool hex_decode(std::array<std::uint8_t, N> &out, const std::string &in) {
	auto end = std::end(out);
	return hex_decode_impl(in, std::begin(out),
						[=](typename std::array<std::uint8_t, N>::iterator it) {
		return it == end; });
}

inline std::string hex_decode(const std::string &in) {
	std::string out;
	out.reserve(20);
	hex_decode_impl(in, std::back_inserter(out),
					[](std::back_insert_iterator<std::string>) { return false; });
	return out;
}
std::string bintohex(const std::string &in);

#endif
