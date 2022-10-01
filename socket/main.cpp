#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <poll.h>

using namespace std;

template <typename T>
void ft_bzero(T *s, size_t size)
{
	for (size_t i = 0; i < size; ++i)
	{
		*s = (T)0;
		++s;
	}
}

int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		cout << "Error: socket()" << endl;
		return (1);
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cout << "Error: bind()" << endl;
		return (1);
	}

	if (listen(sockfd, SOMAXCONN) < 0)
	{
		cout << "Error: listen()" << endl;
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in get_addr;
	socklen_t len = sizeof(struct sockaddr_in);

	struct pollfd fds[1];

	memset(&fds, 0, sizeof(fds));
	fds[0].fd = sockfd;
	fds[0].events = POLLIN; // | POLLERR;
	char buf[300];
	while (1)
	{
		poll(fds, 1, -1);
		if (fds[0].revents & POLLIN)
		{
			cout << "here" << endl;
			int connect = accept(sockfd, (struct sockaddr *)&get_addr, &len);
			if (connect < 0)
			{
				cout << "Error: accept()" << endl;
				exit(1);
			}
			int recv_ret = recv(connect, buf, sizeof(buf), 0);
			cout << buf;
			int read_sum = recv_ret;
			while (recv_ret == sizeof(buf))
			{
				recv_ret = recv(connect, buf, sizeof(buf), 0);
				cout << buf;
				read_sum += recv_ret;
			}
			cout << endl
				 << "read: " << read_sum << "byte, done" << endl;
			send(connect, "hello world", 11, 0);
			close(connect);
		}
	}
	close(sockfd);
}
