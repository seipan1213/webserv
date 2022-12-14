
// #define _GNU_SOURCE   // 一部のオペレーティング・システムでは、_GNU_SOURCEの定義は不要。(https://docs.oracle.com/cd/E39368_01/porting/ch03s06.html)
#include "socket.hpp"

namespace ft
{
	Socket::Socket() : sockfd_vec_(), closedfd_vec_(), poll_fd_vec_(), fd_to_index_nap_(), last_recieve_time_map_(),
		msg_to_send_map_(), fd_to_port_map_(), used_fd_set_(), port_num_(), keep_connect_time_len_(10)
	{
	}

	Socket::~Socket()
	{
		closeAllSocket_();
	}

	Socket::RecievedMsg::RecievedMsg()
		: content(""), client_id(0), port(0), i_poll_fd(0)
	{
	}

	Socket::RecievedMsg::RecievedMsg(const RecievedMsg& src)
		: content(src.content), client_id(src.client_id), port(src.port), i_poll_fd(src.i_poll_fd)
	{
	}

	Socket::RecievedMsg::RecievedMsg(const std::string content, const int client_id, in_port_t port, size_t i_poll_fd)
		: content(content), client_id(client_id), port(port), i_poll_fd(i_poll_fd)
	{
	}

	Socket::RecievedMsg Socket::RecievedMsg::operator=(const Socket::RecievedMsg &other)
	{
		if (this == &other)
			return (*this);

		content = other.content;
		client_id = other.client_id;
		port = other.port;
		i_poll_fd = other.i_poll_fd;
		return (*this);
	}
	
	Socket::RecievedMsg::~RecievedMsg()
	{
	}

	void Socket::setup(const std::vector<ServerConfig> &server_config_vec)
	{
		struct sockaddr_in server_sockaddr;
		struct pollfd poll_fd;
		std::set<unsigned int> boundPorts;
		unsigned int listenPort = 0;

		for (size_t i = 0; i < server_config_vec.size(); ++i)
		{
			listenPort = server_config_vec[i].getListen();
			if (boundPorts.find(listenPort) != boundPorts.end()) {
				continue ;
			}

			sockfd_vec_.push_back(socket(AF_INET, SOCK_STREAM, 0));
			if (sockfd_vec_.back() < 0)
				throw SetUpFailException("Error: socket()");

			set_sockaddr_(server_sockaddr, "127.0.0.1", listenPort);
			std::cout << "127.0.0.1 " << listenPort << " " << sockfd_vec_.back() << std::endl;

			const int opt = 1;
			if (setsockopt(sockfd_vec_.back(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
				throw SetUpFailException("Error: setsockopt");
			}
			if (setsockopt(sockfd_vec_.back(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
				throw SetUpFailException("Error: setsockopt");
			}
	
			int flags = fcntl(sockfd_vec_.back(), F_GETFL, 0);
			if (flags == -1)
				throw (std::runtime_error("fcntl() system error"));
			if (!(flags & O_NONBLOCK)) {
				fcntl(sockfd_vec_.back(), F_SETFL, (flags | O_NONBLOCK));
			}

			fd_to_port_map_[sockfd_vec_.back()] = listenPort;

			if (bind(sockfd_vec_.back(), (struct sockaddr *)&server_sockaddr,
					sizeof(server_sockaddr)) < 0)
				throw SetUpFailException("Error: bind()");

			if (listen(sockfd_vec_.back(), SOMAXCONN) < 0)
				throw SetUpFailException("Error: listen()");

			boundPorts.insert(listenPort);

			poll_fd.fd = sockfd_vec_.back();
			poll_fd.events = POLLIN;
			poll_fd.revents = 0;
			poll_fd_vec_.push_back(poll_fd);
			last_recieve_time_map_[sockfd_vec_.back()] = -1;
		}
	}

	/*void Socket::erase_pollfd(const int fd)
	{
		size_t index = 0;
		for (; index < poll_fd_vec_.size() && poll_fd_vec_[index].fd != fd; ++index) { ; }
		poll_fd_vec_.erase(poll_fd_vec_.begin() + index);	
	}

	void Socket::erase_pollfd_by_index(const int index)
	{
		fd_to_index_nap_.erase(poll_fd_vec_[index].fd);
		poll_fd_vec_.erase(poll_fd_vec_.begin() + index);
	}*/

	Socket::RecievedMsg Socket::recieve_msg()
	{	
		std::cout << "poll_fd_vec_.size(): " << poll_fd_vec_.size() << std::endl;
		for (size_t i = 0; i < poll_fd_vec_.size(); ++i) {
			std::cout << poll_fd_vec_[i].fd << " e" << poll_fd_vec_[i].events << " re" << poll_fd_vec_[i].revents;
			std::cout << (i < poll_fd_vec_.size() - 1 ? " : " : "");
		}
		std::cout << std::endl;

		poll(&poll_fd_vec_[0], poll_fd_vec_.size(), 1000);

		std::cout << "poll done" << std::endl;
		for (size_t i = 0; i < poll_fd_vec_.size(); ++i)
		{
			if (poll_fd_vec_[i].revents & POLLERR)
			{
				std::cerr << "POLLERR: " << poll_fd_vec_[i].fd << std::endl;	
				close_fd_(poll_fd_vec_[i].fd, i);
				throw connectionHangUp(poll_fd_vec_[i].fd);
			}
			else if (poll_fd_vec_[i].revents & POLLHUP)
			{
				std::cerr << "POLLHUP: " << poll_fd_vec_[i].fd << std::endl;
				close_fd_(poll_fd_vec_[i].fd, i);
				throw connectionHangUp(poll_fd_vec_[i].fd);
			}
			else if (poll_fd_vec_[i].revents & POLLRDHUP)
			{
				std::cerr << "POLLRDHUP: " << poll_fd_vec_[i].fd << std::endl;
				close_fd_(poll_fd_vec_[i].fd, i);	
				throw connectionHangUp(poll_fd_vec_[i].fd);
			}
			else if (poll_fd_vec_[i].revents & POLLIN)
			{
				poll_fd_vec_[i].revents = 0;
				if (used_fd_set_.count(poll_fd_vec_[i].fd))
				{
					poll_fd_vec_[i].revents = 0;
					return (recieve_msg_from_connected_client_(poll_fd_vec_[i].fd, i));
				}
				else
				{
					register_new_client_(poll_fd_vec_[i].fd);
					throw recieveMsgFromNewClient(*(--used_fd_set_.end()));
				}
			}
			else if (poll_fd_vec_[i].revents & POLLOUT)
			{
				poll_fd_vec_[i].revents = 0;
				std::string &msg_to_send = msg_to_send_map_[poll_fd_vec_[i].fd];
				ssize_t sent_num = send(poll_fd_vec_[i].fd, msg_to_send.c_str(),
									   msg_to_send.size(), 0);
				if (sent_num == -1)
					throw std::runtime_error("send() system error");
				msg_to_send_map_.erase(poll_fd_vec_[i].fd);
				poll_fd_vec_[i].events = POLLIN;
				last_recieve_time_map_[poll_fd_vec_[i].fd] = time(NULL);
			}
		}
		// throw recieveMsgException();	// pollにタイムアウトを設定するので除外
		throw NoRecieveMsg();
	}

	void Socket::send_msg(int fd, const std::string msg)
	{
		msg_to_send_map_[fd].append(msg);	
		size_t index = 0;
		for (; index < poll_fd_vec_.size() && poll_fd_vec_[index].fd != fd; ++index) { ; }

		poll_fd_vec_[index].events = POLLOUT;
	}

	std::vector<int>& Socket::check_keep_time_and_close_fd()
	{
		time_t current_time = time(NULL);
		time_t tmp_last_recieve_time;

		std::cout << current_time << std::endl;

		for (size_t i = 0; i < poll_fd_vec_.size(); ++i)
		{
			tmp_last_recieve_time = last_recieve_time_map_[poll_fd_vec_[i].fd];
			if (tmp_last_recieve_time != (time_t)-1)
			{ // fd made by accept(), not sockfd
				if (current_time - tmp_last_recieve_time > keep_connect_time_len_)
				{
					std::cerr << "keep alive close" << std::endl;
					closedfd_vec_.push_back(poll_fd_vec_[i].fd);
					close_fd_(poll_fd_vec_[i].fd, i);
				}
			}
		}
		return(closedfd_vec_);
	}

	void Socket::register_new_client_(int sock_fd)
	{
		int connection = accept(sock_fd, NULL, NULL);
		if (connection < 0)
			throw SetUpFailException("Error: accept()");

		int flags = fcntl(connection, F_GETFL, 0);
		if (flags == -1)
			throw (std::runtime_error("fcntl() system error"));
		if (!(flags & O_NONBLOCK)) {
			fcntl(connection, F_SETFL, (flags | O_NONBLOCK));
		}

		struct pollfd poll_fd;
		poll_fd.fd = connection;
		poll_fd.events = POLLIN;
		poll_fd.revents = 0;
		poll_fd_vec_.push_back(poll_fd);
		used_fd_set_.insert(connection);

		last_recieve_time_map_[connection] = time(NULL);
		fd_to_port_map_[connection] = fd_to_port_map_[sock_fd];
	}

	Socket::RecievedMsg Socket::recieve_msg_from_connected_client_(int connection, size_t i_poll_fd)
	{
		char buf[BUFFER_SIZE + 1];

		last_recieve_time_map_[connection] = time(NULL);
		int recv_ret = recv(connection, buf, BUFFER_SIZE, MSG_DONTWAIT);
		buf[recv_ret] = '\0';
		if (recv_ret == -1)
			std::runtime_error("idk");
		if (recv_ret == 0) {
			close_fd_(connection, i_poll_fd);
			throw connectionHangUp(connection);
		}
		return (RecievedMsg(std::string(buf), connection, fd_to_port_map_[connection], i_poll_fd));
	}

	void Socket::close_fd_(const int fd, const int i_poll_fd)
	{
		close(fd);
		poll_fd_vec_.erase(poll_fd_vec_.begin() + i_poll_fd);
		used_fd_set_.erase(fd);
		fd_to_port_map_.erase(fd);
	}

	void Socket::closeAllSocket_()
	{
		for (size_t i = 0; i < port_num_; ++i)
			close(poll_fd_vec_[i].fd);
		poll_fd_vec_.clear();
		used_fd_set_.clear();
		fd_to_port_map_.clear();
	}

	void Socket::set_sockaddr_(struct sockaddr_in &server_sockaddr, const char *ip, const in_port_t port)
	{
		memset(&server_sockaddr, 0, sizeof(struct sockaddr_in));
		server_sockaddr.sin_family = AF_INET;
		server_sockaddr.sin_port = htons(port);
		server_sockaddr.sin_addr.s_addr = inet_addr(ip);
	}

	Socket::SetUpFailException::SetUpFailException(const std::string err_msg)
		: err_msg(err_msg)
	{
	}

	Socket::SetUpFailException::~SetUpFailException() throw()
	{
	}

	const char *Socket::SetUpFailException::what() const throw()
	{
		return (err_msg.c_str());
	}

	const char *Socket::RecieveMsgException::what() const throw()
	{
		return ("Error: recieve msg fail, shouldn't happen");
	}

	Socket::recieveMsgFromNewClient::recieveMsgFromNewClient(const int client_id)
		: client_id(client_id)
	{
	}

	Socket::connectionHangUp::connectionHangUp(const int client_id)
		: client_id(client_id)
	{
	}
}
