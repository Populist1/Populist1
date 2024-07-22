// C++ program to illustrate the client application in the 
// socket programming 
#include <cstring> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <thread>
#include <chrono>

void process_buffer(char* buffer)
{
	int size = strlen(buffer);
	if (size % 32 == 0)
	{
		std::cout << buffer;
		std::cout.flush();
	}
	else
	{
		std::cout << "ERROR" << std::endl;
	}
}

int main() 
{ 
	// creating socket 
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0); 

	// specifying address 
	sockaddr_in serverAddress; 
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(8080); 
	serverAddress.sin_addr.s_addr = INADDR_ANY; 

	// sending connection request 
	int error_code = connect(clientSocket, (struct sockaddr*)&serverAddress,
			sizeof(serverAddress)); 

	while (error_code == -1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		error_code = connect(clientSocket, (struct sockaddr*)&serverAddress,
			sizeof(serverAddress));
	}

	while (true)
	{
		// sending data
		const char* message = "\n";
		while (true)
		{
			error_code = send(clientSocket, message, strlen(message), MSG_NOSIGNAL);
			if (error_code != -1)
			{
				break;
			}
			while (error_code == -1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				close(clientSocket);
				clientSocket = socket(AF_INET, SOCK_STREAM, 0);
				error_code = connect(clientSocket, (struct sockaddr*)&serverAddress,
				sizeof(serverAddress));
			}
		}

		// получаем данные
		char buffer[1024] = { 0 };
		recv(clientSocket, buffer, sizeof(buffer), 0);
		process_buffer(buffer);
	}


	// closing socket 
	close(clientSocket); 

	return 0; 
}
