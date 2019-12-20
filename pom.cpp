#include <fcntl.h>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <ezpwd/rs>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

// Reed Solomon codec w/ 255 symbols, up to 251 data, 4 parity symbols
ezpwd::RS<255,251> rs;

std::vector<uint8_t> encode(const std::vector<uint8_t> & in)
{
	std::vector<uint8_t> work = in;
	rs.encode(work);

	int n_bits = 0;
	uint8_t bits = 0;

	std::vector<uint8_t> out;
	for(size_t i=0; i<work.size(); i++) {
		uint8_t b = work.at(i);

		for(int bit=0; bit<8; bit++) {
			bits <<= 1;
			bits |= !!(b & 128);
			b <<= 1;
			n_bits++;

			if (n_bits == 5) {
				out.push_back(bits);
				bits = 0;
				n_bits = 0;
			}
		}
	}

	if (n_bits) {
		printf("# left: %d\n", n_bits);
		bits <<= 5 - n_bits;
		out.push_back(bits);
	}

	if (!out.empty())
		out.push_back(63);

	return out;
}

std::vector<uint8_t> decode(const std::vector<uint8_t> & in)
{
	std::vector<uint8_t> work = in;

	int n_bits = 0, nbo = 0;
	uint8_t bits = 0, bo = 0;

	std::vector<uint8_t> out;
	size_t i = 0;
	for(;;) {
		if (n_bits == 0) {
			if (i == work.size()) // should not be reached
				break;

			n_bits = 5;
			bits = work.at(i++);

			if (bits == 63) {
				nbo = 0;
				break;
			}
		}

		bo <<= 1;
		bo |= !!(bits & 16);
		bits <<= 1;
		n_bits--;

		nbo++;
		if (nbo == 8) {
			out.push_back(bo);
			nbo = 0;
			bo = 0;
		}
	}

	printf("rs-decode: %d\n", rs.decode(out));

	out.resize(out.size() - rs.nroots());

	if (nbo)
		printf("fail: %d\n", nbo);

	return out;
}

std::string encoded_to_asc(const std::vector<uint8_t> & in)
{
	std::string txt;

	for(int c : in) {
		if (c < 26)
			c += 'A';
		else if (c < 36)
			c += '0' - 26;
		else if (c == 63)
			c = '/';
		else
			printf("encode error: %d\n", c);

		txt += c;
	}

	return txt;
}

std::vector<uint8_t> asc_to_encoded(const std::string & in)
{
	std::vector<uint8_t> out;

	for(int c : in) {
		c = toupper(c);

		if (c >= 'A' && c <= 'Z')
			c -= 'A';
		else if (c >= '0' && c <= '9') {
			c -= '0';
			c += 26;
		}
		else if (c == '/')
			c = 63;
		else
			printf("decode error: %d\n", c);

		out.push_back(c);
	}

	return out;
}

#ifdef TEST
int main(int arc, char *argv[])
{
	std::string in = "Hallo, wereld!";
	std::vector<uint8_t> bytes(in.begin(), in.end());

	std::vector<uint8_t> encoded = encode(bytes);

	printf("%zu -> %zu\n", in.size(), encoded.size());
	for(size_t i=0; i<encoded.size(); i++)
		printf("%d ", encoded.at(i));
	printf("\n");

	std::string txt = encoded_to_asc(encoded);
	printf("%s\n", txt.c_str());

	std::vector<uint8_t> bin = asc_to_encoded(txt);

	std::vector<uint8_t> decoded = decode(bin);
	std::string str_out(decoded.begin(), decoded.end());

	printf("%s\n", str_out.c_str());

	return 0;
}
#else
int main(int argc, char *argv[])
{
	int fd = open("/dev/net/tun", O_RDWR);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	struct ifreq ifr = { 0 };
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if (ioctl(fd, TUNSETIFF, (void *)&ifr) == -1) {
		perror("ioctl(TUNSETIFF)");
		return 1;
	}

	char buffer[65536];

	std::vector<uint8_t> bin = asc_to_encoded("IUAAAVAT3BAAAQABCK5QUAAAAEFAAAACBAAJHQYVZIAACEQ45VOQAAAAAB0CEDAAAAAAAAAQCEJBGFAVCYLRQGI0DMOB0HQ5EAQSEIZEEUTCOKBJFIVSYLJOF2YDCMRTGQ0TMN0TRHVAE/");
	std::vector<uint8_t> decoded = decode(bin);
	for(size_t i=0; i<decoded.size(); i++)
		buffer[i] = decoded.at(i);

	write(fd, buffer, decoded.size());

	for(;;) {
		int rc = read(fd, buffer, sizeof(buffer));
		if (rc <= 0)
			break;

		std::vector<uint8_t> bytes;
		bytes.insert(bytes.end(), buffer, buffer + rc);
		std::vector<uint8_t> encoded = encode(bytes);
		std::string txt = encoded_to_asc(encoded);
		printf("%s\n", txt.c_str());
	}

	return 0;
}

#endif
