#pragma once

#ifndef __TCPSERVER_HPP_INCLUDED__
#define __TCPSERVER_HPP_INCLUDED__

class CommandMessageProtocol;
using CMPROTO = CommandMessageProtocol;

using msg_queue_t = std::deque<CMPROTO*>;

class TCPServer
{
public:
	TCPServer(boost::asio::io_context &io_context, const boost::asio::ip::tcp::endpoint &endpoint);
	~TCPServer();

	void accept();
	void write(const CMPROTO *msg);

private:
	void read_header();
	void read_body();
	void write();

private:
	boost::asio::io_context        &m_io;
	boost::asio::ip::tcp::socket    m_socket;
	boost::asio::ip::tcp::acceptor  m_acceptor;
	CMPROTO                        *m_read_msg;
	msg_queue_t                     m_write_msgs;
};

#endif /* __TCPSERVER_HPP_INCLUDED__ */