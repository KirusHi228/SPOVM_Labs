#include <iostream>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <aio.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 80

using namespace std;

bool checkFormat(char* str)
{
	int i = 0;
	char identificator[] = "txt.";

	while (str[i + 1]) i++;

	for (int j = 0; j < 4; j++)
		if (str[i - j] != identificator[j]) return 0;

	return 1;
}

bool checkString(char* str)
{
	if (!checkFormat(str))
		return 0;

	if (!strcmp(str, "output.txt"))
		return 0;

	return  1;
}

void* writeThread(void* ptr)
{

	int pipe = *(int*)ptr; 

	char buffer[BUF_SIZE];
	ssize_t pipeRead;  

	int sem = semget(ftok("./main.cpp", 1), 0, IPC_CREAT | 0666);
	struct sembuf wait = { 0, 0, SEM_UNDO };
	struct sembuf lock = { 0, 1, SEM_UNDO };
	struct sembuf unlock = { 1, -1, SEM_UNDO }; 
	semop(sem, &lock, 1);

	int filePtr;  
	filePtr = open("output.txt", O_WRONLY | O_TRUNC | O_CREAT); 

	aiocb writeFile;  
	writeFile.aio_fildes = filePtr; 
	writeFile.aio_offset = 0; 
	writeFile.aio_buf = &buffer;  

	while (1)
	{
		semop(sem, &wait, 1);  
		semop(sem, &lock, 1);

		pipeRead = read(pipe, buffer, BUF_SIZE);   
		if (!pipeRead) break;

		writeFile.aio_nbytes = pipeRead;  
		aio_write(&writeFile);  

		while (aio_error(&writeFile) == EINPROGRESS); 
			writeFile.aio_offset += pipeRead; 

		semop(sem, &unlock, 1);   
	}

}

//Поток-читатель
void* reader(void* ptr)
{
	int pipe = *(int*)ptr;  

	char buffer[BUF_SIZE]; 

	int sem = semget(ftok("./main.cpp", 1), 2, IPC_CREAT | 0666);    
	struct sembuf wait = { 1, 0, SEM_UNDO };  
	struct sembuf lock = { 1, 1, SEM_UNDO };   
	struct sembuf unlock = { 0, -1, SEM_UNDO };  

	aiocb readFile;           
	readFile.aio_buf = &buffer; 

	DIR* directory;     
	dirent* currentFile;   
	directory = opendir("/home/user/CLionProjects/SPOVM5/");   

	int filePtr;  
	struct stat stat;   
	int stringSize;

	while (1)
	{

		currentFile = readdir(directory);  

		if (currentFile == NULL) break; 

		if (!checkString(currentFile->d_name)) 
			continue;

		filePtr = open(currentFile->d_name, O_RDONLY); 

		fstat(filePtr, &stat);   
		stringSize = stat.st_size; 

		readFile.aio_fildes = filePtr; 
		readFile.aio_offset = 0;  


		while (1)
		{
			if (stringSize > BUF_SIZE) readFile.aio_nbytes = BUF_SIZE;
			else readFile.aio_nbytes = stringSize;

			aio_read(&readFile); 

			while (aio_error(&readFile) == EINPROGRESS); 

			semop(sem, &wait, 1); 
			semop(sem, &lock, 1);

			write(pipe, buffer, readFile.aio_nbytes);  

			semop(sem, &unlock, 1); 

			if (stringSize > BUF_SIZE)
			{
				stringSize -= BUF_SIZE;
				readFile.aio_offset += BUF_SIZE;
			}
			else 
				break;
		}

		close(filePtr);  
	}

	semop(sem, &wait, 1);  
}

extern "C" void concat() 
{

	int hndl[2];        

	if (pipe(hndl))
	{
		cout << "The pipe couldn't have been created!" << endl;
		return;
	}


	pthread_t writer_thread;
	if (pthread_create(&writer_thread, NULL, writeThread, &hndl[0]))
	{
		cout << "The writing thread couldn't have been created!" << endl;
		return;
	}


	pthread_t reader_thread;
	if (pthread_create(&reader_thread, NULL, reader, &hndl[1]))
	{
		cout << "The reading thread couldn't have been created!" << endl;
		pthread_cancel(writer_thread);
		return;
	}

	pthread_join(reader_thread, NULL); 
	pthread_cancel(writer_thread); 

	return;
}
