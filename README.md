<h1 align="center">
  NetChat
</h1>
<p align="center">
  <img src="/netchatLogo.png" alt="NetChat Logo" width="150">
</p>


NetChat is a console-based chat application that allows users to communicate with each other over TCP and UDP connections. The application is written in C++ and uses sockets to establish connections between clients. Users can send and receive text messages in real-time, as well as view their chat history. The application includes error handling and input validation to ensure a smooth user experience.


## Features
- Supports both TCP and UDP connections
- Real-time messaging
- Chat history
- Robust error handling and input validation
- Simple and intuitive user interface
## Technologies Used
- C++
- Sockets
- TCP/UDP protocols
- Git
## Learning Outcomes
During the development of NetChat, I gained experience in the following areas:

- Project management
- Networking protocols
- Socket programming
- Error handling and input validation
- NetChat is a great showcase of my skills in C++, networking, and project management. As I continue to develop and improve the application, I look forward to exploring new features and technologies, and applying what I've learned to future projects.

## Installation
To install NetChat, follow these steps:

1. Clone the repository to your local machine:

`git clone https://github.com/Miguel-202/NetChat.git`

2. Navigate to the project directory:

`cd NetChat`

3. Compile the source code using your preferred C++ compiler:

`g++ -o NetChat NetChat.cpp`

4. Run the application:

`./NetChat`
## Usage
Once you've installed NetChat, you can use the following commands to start a chat session:

- `$register <username>`: Registers the specified username for the current client session.
- `$getlist`: Returns a list of all connected clients.
- `$getlog`: Returns the chat log for the current session.
- `$exit`: Closes the connection to the server and exits the application.
- `$chat <message>`: Sends a message to all connected clients.
- Any other message: Sends a message to all connected clients.

To simulate a force quit, enter `$quit` during a chat session.
## License
NetChat is licensed under the MIT License.

##Contributions
Contributions to NetChat are welcome! If you'd like to contribute, please submit a pull request. Before submitting a pull request, please make sure your code follows the existing coding conventions and passes all tests.

## Screenshots

### TCP Connection Setup

![TCP Connection Setup Screenshot](/TCP_ConnectionSetup_Screenshot.png)

This screenshot shows the TCP connection setup process for NetChat.

### UDP Broadcast

![UDP Broadcast Screenshot](/UDP_Broadcast_Screenshot.png)

This screenshot shows NetChat in action, broadcasting a message to all connected clients over UDP.
