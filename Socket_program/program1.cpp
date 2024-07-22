#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <string>
#include <string.h>
#include <algorithm>
#include <fstream>

struct data_chunk
{
	int number;
};

#define buffer_size 64
struct Buffer
{
	unsigned char raw[buffer_size * 2] = {};  // кольцевой буфер
	int write_pos = 0;  // место в буфере, куда надо записывать данные
	int read_pos = 0;  // место в буфере, откуда надо считывать данные
	// read_pos либо "отстаёт" (когда есть непрочитанные данные)
	// , либо равен write_pos (когда буфер пуст)
	int read_available = 0;  // сколько ещё байт можно считать
	int write_available = buffer_size;  // сколько ещё байт можно записать
	bool write(const unsigned char* data, int size)  // записать данные
	{
		if (size > write_available)  // если не поместятся
		{
			return false;  // то возвращаем "ложь"
		}
		memcpy(raw + write_pos, data, size);  // записываем данные в буфер
		write_pos += size;
		if (write_pos > buffer_size)  // если часть данных за пределами буфера
		{
			memcpy(raw, raw + buffer_size, write_pos - buffer_size);  // кидаем её в начало
		}
		write_pos %= buffer_size;  // приводим в порядок позицию записи
		write_available -= size;
		read_available += size;
		return true;
	}
	bool read(unsigned char* data, int size)  // считать данные
	{
		if (size > read_available)  // если запрошено больше данных, чем есть
		{
			return false;  // то возвращаем "ложь"
		}
		int limit = read_pos + size;
		if (limit > buffer_size)  // если данные "дают круг" по буферу
		{
			int first_chunk = buffer_size - read_pos;  // размер первого куска
			memcpy(data, raw + read_pos, first_chunk);  // считываем первый кусок
			memcpy(data + first_chunk, raw, limit - buffer_size);  // второй кусок
		}
		else  // если данные красиво лежат в буфере в одну линию
		{
			memcpy(data, raw + read_pos, size);
		}
		read_pos += size;
		read_pos %= buffer_size;
		read_available -= size;
		write_available += size;
		return true;
	}
	int read_line(unsigned char* data, char stop_char = '\n')
	{
		int found = 0;  // через сколько символов в буфере будет стоп-символ
		while (found < read_available)
		{
			unsigned char cur_char = raw[(read_pos + found) % buffer_size];
			if (cur_char == stop_char)
			{
				found++;
				break;
			}
			found++;
		}
		read(data, found);
		return found;
	}
	int write_line(const unsigned char* data, char stop_char = '\n')
	{
		int found = 0;  // через сколько символов в пользовательской строке бдует стоп-символ
		while (found < write_available)
		{
			if (data[found] == stop_char)
			{
				found++;
				break;
			}
			found++;
		}
		write(data, found);
		return found;
	}
};

bool flag;
std::mutex m;
void wait_for_flag()
{
	std::unique_lock<std::mutex> lk(m);
	while (!flag)
	{
		lk.unlock();  // Разблокировка мьютекса
		std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Вхождение в спящий режим на 100 мс
		lk.lock();  // Повторная блокировка мьютекса
	}
}


#define ENABLE_LOOP


#include <iostream>
#include <queue>
#include <condition_variable>
using namespace std;
std::mutex mut;
std::queue<data_chunk> data_queue;
std::condition_variable data_cond;
bool more_data_to_prepare()
{
	data_chunk example = { rand() };
	static int seed = 0;
	srand(seed);
	int random_number = rand();
	seed = random_number;
	if (random_number % 100 < 80)
	{
		data_queue.push(example);
		return true;
	}
	return false;
}
data_chunk prepare_data()
{
	data_chunk front_data_chunk = data_queue.front();
	data_queue.pop();
	return front_data_chunk;
}
std::condition_variable new_data;

void data_preparation_thread() {
#ifdef ENABLE_LOOP
	while (more_data_to_prepare()) {
#endif
		data_chunk const data = prepare_data();
		{
			std::lock_guard<std::mutex> lk(mut);
			data_queue.push(data);
		}
		data_cond.notify_one();
#ifdef ENABLE_LOOP
	}
#endif
}

void process(data_chunk data, int processor)
{
	std::cout << "Barber " << processor << " serves client " << data.number << "... Zzzz...";
	std::cout << " " << data_queue.size() << " clients left" << std::endl;
}
bool is_last_chunk(data_chunk data)
{
	return data_queue.size() == 0;
}

void data_processing_thread() {
	static int processor_count = 0;
	processor_count++;
	int processor = processor_count;
#ifdef ENABLE_LOOP
	while (true) {
#endif
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [] {return !data_queue.empty(); });
		data_chunk data = data_queue.front();
		data_queue.pop();
		lk.unlock();
		process(data, processor);
		if (false && is_last_chunk(data))
#ifdef ENABLE_LOOP
			break;
#else
			return;
#endif
		//std::this_thread::sleep_for(std::chrono::seconds(5));
#ifdef ENABLE_LOOP
	}
#endif
}


int old_main()
{
	std::cout << "Hello, world!" << std::endl;
	int choice = 0;
	std::cout << "0 - data_preparation" << std::endl;
	std::cout << "1 - data_processing" << std::endl;
	bool to_continue = true;
	while (to_continue)
	{
		std::cout << "which? ";
		std::cin >> choice;
		if (choice == 0)
		{
			std::thread my_thread(data_preparation_thread);
			my_thread.detach();
		}
		else if (choice == 1)
		{
			std::thread my_thread(data_processing_thread);
			my_thread.detach();
		}
		else
		{
			to_continue = false;
		}
	}
	return 0;
}



Buffer buf;


void user_input(std::string& user_string)
{
	std::cout << "Enter your string: ";
	std::getline(std::cin, user_string);
}

std::string trash;
void thread1() {
	std::getline(std::cin, trash);
#ifdef ENABLE_LOOP
	while (true) {
#endif
		std::string data;
		user_input(data);
		{
			std::lock_guard<std::mutex> lk(mut);
			if (data.size() <= 64 && data.size() > 0)
			{
				bool second_check = true;
				for (size_t i = 0; i < data.size(); i++)
				{
					if ((data[i] > '9') || (data[i] < '0'))
					{
						second_check = false;
						break;
					}
				}
				if (second_check)
				{
					std::sort(data.begin(), data.end(), greater<>()); // сортируем по убыванию
					Buffer temp_buf;
					temp_buf.write((unsigned char*)data.c_str(), data.size());
					for (size_t i = 0; i < data.size(); i++)
					{
						unsigned char cur_char;
						temp_buf.read(&cur_char, 1);
						if (cur_char % 2 == 0)
						{
							temp_buf.write((unsigned char*)"KB", 2);
						}
						else
						{
							temp_buf.write(&cur_char, 1);
						}
					}
					unsigned char regular_buffer[buffer_size + 2];
					int new_size = temp_buf.read_available;
					regular_buffer[new_size] = '\n';
					temp_buf.read(regular_buffer, new_size);
					new_size++;
					buf.write(regular_buffer, new_size);
					//buf.write((unsigned char*)data.c_str(), data.size());
				}
			}
		}
		data_cond.notify_one();
#ifdef ENABLE_LOOP
	}
#endif
}


#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

//ofstream output_file("processor_output.txt");
void send_to_program2(char* regular_buffer, int size, int processor)
{
	if (size > 0) {
		regular_buffer[size] = '\0';
	}
	//std::cout << "Processor " << processor << " says: "/* << regular_buffer*/;
	for (int i = 0; i < size; i++)
	{
		//std::cout << (int)(unsigned char)regular_buffer[i] << "\t";
	}
	std::cout << std::endl;
	std::cout.flush();

	// Посылаем через сокет второй программе

	static bool initialized = false;  // инициализирован ли сокет

	// создаём сокет
	static int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (!initialized)
	{
		if (serverSocket == -1) {
			std::cerr << "Can't create a socket!" << std::endl;
			return;
		}
	}

	// указываем адрес
	static sockaddr_in serverAddress;
	if (!initialized)
	{
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(8080);
		serverAddress.sin_addr.s_addr = INADDR_ANY;
	}

	// переиспользуем порт 8080
	int reusePort = 1;
	// переопределяем поведение сокета
	if (!initialized)
	{
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reusePort, sizeof(reusePort));
	}

	// прикрепляем сокет
	if (!initialized)
	{
		if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
			std::cerr << "Can't bind socket!" << std::endl;
			return;
		}
	}

	// слушаем назначенный сокет
	if (!initialized)
	{
		if (listen(serverSocket, 5) == -1) {
			std::cerr << "Can't listen on socket!" << std::endl;
			return;
		}
	}

	// принимаем запрос на подключение
	static int clientSocket = accept(serverSocket, nullptr, nullptr);
	bool went_back = false;
	go_back:
	if (!initialized || went_back)
	{
		if (clientSocket == -1) {
			std::cerr << "Can't accept connection!" << std::endl;
			return;
		}
		went_back = false;
	}

	// получаем данные
	char buffer[1024] = { 0 };
	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		//std::cout << "Message from client: " << buffer << std::endl;
	}
	else {
		clientSocket = accept(serverSocket, nullptr, nullptr);
		went_back = true;
		goto go_back;
	}

	send(clientSocket, regular_buffer, size, 0);

	// закрываем сокеты
	if (false)
	{
		close(clientSocket);
		close(serverSocket);
	}
	initialized = true;
}

/*
void send_to_program2(char* regular_buffer, int size, int processor)
{
	regular_buffer[size] = '\0';
	std::cout << "Processor " << processor << " says: ";// << regular_buffer;
	for (int i = 0; i < size; i++)
	{
		std::cout << (int)(unsigned char)regular_buffer[i] << "\t";
	}
	std::cout << std::endl;
	std::cout.flush();

	// Посылаем через сокет второй программе

	// создаём сокет
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	// указываем адрес
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// прикрепляем сокет
	bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	// слушаем назначенный сокет
	listen(serverSocket, 5);

	// принимаем запрос на подключение
	int clientSocket = accept(serverSocket, nullptr, nullptr);

	// получаем данные
	char buffer[1024] = { 0 };
	recv(clientSocket, buffer, sizeof(buffer), 0);
	std::cout << "Message from client: " << buffer << std::endl;

	send(clientSocket, regular_buffer, size, 0);

	// закрываем сокет
	close(clientSocket);
	close(serverSocket);
}
*/

void thread2() {
	static int processor_count = 1;
	processor_count++;
	int processor = processor_count;
#ifdef ENABLE_LOOP
	while (true) {
#endif
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [] {
			//std::cout << "Ding! " << buf.read_available << " bytes available." << std::endl;
			//printf("lalala\n");
			//ofstream output_file("output.txt");
			//output_file << "Hello, world!" << std::endl;
			//output_file.close();
			return buf.read_available != 0;
			});
		char regular_buffer[buffer_size + 2];
		int size = buf.read_line((unsigned char*)regular_buffer);
		lk.unlock();
		send_to_program2(regular_buffer, size, processor);
		if (false && buf.read_available == 0)
#ifdef ENABLE_LOOP
			break;
#else
			return;
#endif
		//std::this_thread::sleep_for(std::chrono::seconds(5));
#ifdef ENABLE_LOOP
	}
#endif
}


int buffer_check_main()
{
	int choice;
	std::string temp;
	char buffer[buffer_size + 1];
	Buffer buf;
	while (true)
	{
		std::cout << "0 - read, 1 - write, 2 - debug: ";
		std::cin >> choice;
		if (choice == 0)
		{
			int size = buf.read_line((unsigned char*)buffer);
			buffer[size] = '\0';
			std::cout << buffer;
		}
		else if (choice == 1)
		{
			std::getline(std::cin, temp);
			std::getline(std::cin, temp);
			temp += '\n';
			buf.write_line((unsigned char*)temp.c_str());
		}
		else if (choice == 2)
		{
			std::cout << "read_available =\t" << buf.read_available << std::endl;
			std::cout << "write_available =\t" << buf.write_available << std::endl;
		}
		else
		{
			break;
		}
	}
	return 0;
}


int main(int argc, char** argv)
{
	std::cout << "Hello, world!" << std::endl;
	int choice = 0;
	std::cout << "0 - thread1" << std::endl;
	std::cout << "1 - thread2" << std::endl;
	bool to_continue = true;
	while (to_continue)
	{
		std::cout << "which? ";
		std::cin >> choice;
		if (choice == 0)
		{
			std::thread my_thread(thread1);
			my_thread.join();
		}
		else if (choice == 1)
		{
			std::thread my_thread(thread2);
			my_thread.detach();
		}
		else
		{
			to_continue = false;
		}
	}
	return 0;
}
