# PA-1: FIFO Client-Server Communication

## Project Title
CSCE313 Programming Assignment 1: FIFO Client-Server Communication

## Description
This project implements a client-server communication system using named pipes (FIFOs) in C++ for CSCE313. The server responds to client requests for ECG data, file transfers, and supports dynamic channel creation, demonstrating inter-process communication and synchronization via FIFOs.

## Usage
To build the project, run:
```
make
```
This will produce the `server` and `client` executables.

To run the server:
```
./server -p <main_fifo_name> -t <number_of_threads> -e <max_channels>
```

To run the client:
```
./client -p <main_fifo_name> -f <filename> -c <patient_id> -m <buffer_size>
```
where:
- `-p` specifies the name of the main FIFO for communication.
- `-t` sets the number of worker threads in the server.
- `-e` specifies the maximum number of channels.
- `-f` is the filename for file transfer requests.
- `-c` is the patient ID for ECG data requests.
- `-m` sets the message buffer size.

## Key Functionalities
- **Requesting ECG Data Points**: The client can request ECG data for a specific patient and time point, and the server responds with the requested value.
- **Generating `x1.csv`**: The client can request all ECG data for a patient and generate a CSV file (`x1.csv`) containing the results.
- **File Transfer**: The client can request a file from the server, which is transferred in chunks via the FIFO.
- **Dynamic Channel Creation**: The client can request the creation of new communication channels (FIFOs) for parallel or isolated requests.

## Report
Performance analysis was conducted to measure transfer time versus file size for the file transfer functionality. The results, documented in `answer.txt` and visualized in `times.png`, show that transfer time scales nearly linearly with file size. This behavior is attributed to the overhead of context switching and FIFO operations, which become less significant as file size increases.

Link : https://github.com/aaryabookseller16/CSCE313-PA1.git


