all: server/bin/chat_server

# Main server program

server/bin/chat_server: server/server.c server/chat_server/bin/chat_server.o server/chat_server/bin/chat_server_fsm.o server/chat_server/bin/chat_server_network.o common/message_queue/bin/message_queue.o server/chat_server/api/chat_server.h common/common_types.h common/message_queue/api/message_queue.h common/chat_event/bin/chat_event.o
	gcc -Iserver/chat_server/api -Icommon -Icommon/message_queue/api server/chat_server/bin/chat_server.o server/chat_server/bin/chat_server_fsm.o server/chat_server/bin/chat_server_network.o common/message_queue/bin/message_queue.o common/chat_event/bin/chat_event.o server/server.c -o server/bin/chat_server


# Chat server module

server/chat_server/bin/chat_server.o: server/chat_server/src/chat_server.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_fsm.h common/common_types.h common/message_queue/api/message_queue.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon -Icommon/chat_event/api -Icommon/message_queue/api -c server/chat_server/src/chat_server.c -o server/chat_server/bin/chat_server.o

server/chat_server/bin/chat_server_fsm.o: server/chat_server/src/chat_server_fsm.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_fsm.h common/common_types.h common/message_queue/api/message_queue.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon -Icommon/chat_event/api -Icommon/message_queue/api -c server/chat_server/src/chat_server_fsm.c -o server/chat_server/bin/chat_server_fsm.o

server/chat_server/bin/chat_server_network.o: server/chat_server/src/chat_server_network.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_connections.h common/common_types.h common/message_queue/api/message_queue.h common/chat_event/api/chat_event.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon -Icommon/chat_event/api -Icommon/message_queue/api -Icommon/chat_event/api -c server/chat_server/src/chat_server_network.c -o server/chat_server/bin/chat_server_network.o


# Main client program

client/bin/chat_client: client/client.c common/chat_event/api/chat_event.h
	gcc -Icommon client/client.c -o client/bin/chat_client


# Chat client module

client/chat_client/bin/chat_client_fsm.o: client/chat_client/src/client_fsm.c client/chat_client/inc/client_internal.h
	gcc -Iclient/chat_client/inc -Icommon client/chat_client/src/chat_client_fsm.c


# Message queue module

common/message_queue/bin/message_queue.o: common/message_queue/src/message_queue.c common/message_queue/api/message_queue.h common/message_queue/inc/message_queue_internal.h common/common_types.h
	gcc -Icommon/message_queue/api -Icommon/message_queue/inc -Icommon -c common/message_queue/src/message_queue.c -o common/message_queue/bin/message_queue.o


# Chat event module

common/chat_event/bin/chat_event.o: common/chat_event/src/chat_event.c common/chat_event/api/chat_event.h common/common_types.h
	gcc -Icommon -Icommon/chat_event/api -c common/chat_event/src/chat_event.c -o common/chat_event/bin/chat_event.o
