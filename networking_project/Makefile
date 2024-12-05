all: server/bin/chat_server

# Main server program

server/bin/chat_server: server/main.c server/chat_server/bin/chat_server.o server/chat_server/bin/chat_server_fsm.o server/chat_server/bin/chat_server_network.o common/message_queue/bin/message_queue.o server/chat_server/api/chat_server.h common/inc/common_types.h common/message_queue/api/message_queue.h
	gcc -Iserver/chat_server/api -Icommon/inc -Icommon/message_queue/api server/chat_server/bin/chat_server.o server/chat_server/bin/chat_server_fsm.o server/chat_server/bin/chat_server_network.o common/message_queue/bin/message_queue.o server/main.c -o server/bin/chat_server


# Chat server module

server/chat_server/bin/chat_server.o: server/chat_server/src/chat_server.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_fsm.h common/inc/common_types.h common/message_queue/api/message_queue.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon/inc -Icommon/message_queue/api -c server/chat_server/src/chat_server.c -o server/chat_server/bin/chat_server.o

server/chat_server/bin/chat_server_fsm.o: server/chat_server/src/chat_server_fsm.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_fsm.h common/inc/common_types.h common/message_queue/api/message_queue.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon/inc -Icommon/message_queue/api -c server/chat_server/src/chat_server_fsm.c -o server/chat_server/bin/chat_server_fsm.o

server/chat_server/bin/chat_server_network.o:server/chat_server/src/chat_server_network.c server/chat_server/api/chat_server.h server/chat_server/inc/chat_server_internal.h server/chat_server/inc/chat_server_connections.h common/inc/common_types.h
	gcc -Iserver/chat_server/api -Iserver/chat_server/inc -Icommon/inc -Icommon/message_queue/api -c server/chat_server/src/chat_server_network.c -o server/chat_server/bin/chat_server_network.o


# Message queue module

common/message_queue/bin/message_queue.o: common/message_queue/src/message_queue.c common/message_queue/api/message_queue.h common/message_queue/inc/message_queue_internal.h common/inc/common_types.h
	gcc -Icommon/message_queue/api -Icommon/message_queue/inc -Icommon/inc -c common/message_queue/src/message_queue.c -o common/message_queue/bin/message_queue.o
