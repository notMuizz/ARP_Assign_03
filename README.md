# ARP_Assign_03

[Hafiz Muizz Ahmed Sethi ID: S5428151](https://github.com/notMuizz)<br>
[M.Sc Robotics Engineering](https://corsi.unige.it/corsi/10635)<br>

## Description:
The assignment entails the design and development of a modified version of Assignment 2, incorporating client/server functionalities using sockets. In this modified version, **ProcessB**, shared memory, and the second ncurses window remain unchanged. However, **ProcessA** now offers the capability to establish a connection with either a client or server for a similar application running on a different machine within the network.

Upon launching the application, the user is prompted to select one of the following execution modes:

- *Normal*: The behavior mirrors that of Assignment 2 precisely.
- *Server*: The application no longer relies on keyboard input; instead, it receives input from another application (running in client mode) on a separate machine.
- *Client*: The application functions as per Assignment 2, while additionally sending its keyboard input to another application (running in server mode) on a different machine.

## Processes

The following processes have been implemented:

- **ProcessA** (the same as in Assignment 2)., creating a moving image using an ncurses window. Using the arrow keys, the user will move a circle in the window, producing an image in the simulated, shared, video memory. Additionally, when the user presses the button in the ncurses window, a snapshot of the image will be saved in a .bmp file. **Client** It is a modification of **ProcessA** which, additionally, sends the keyboard input to another application (on a different machine) running in server mode. **Server**, It is a modification of **ProcessA** which, instead of receiving the commands from the keyboard, receives them from another application (on a different machine) running in client mode.
- **ProcessB** (the same as in Assignment 2). It simulates the extraction of the center of the circle from the acquired moving image. Also, it will show the position trace of the center of the circle in a second ncurses window.
- **Master**. It has been modified with respect to the one in Assignment 2. Now, it asks the user to select the execution modality between the previous three options, spawning the appropriate processes in each case.


## Required libraries/packages
### Install Libbitmap
 
 download from https://github.com/draekko/libbitmap

navigate to the root directory of the downloaded repo and type `./configure`

type `make`

run `make install`

### Install ncurses

```console
sudo apt-get install libncurses-dev
```
## Compiling and running the code
In order to compile all the processes that make up the program, you just need to execute the following command:
```console
chmod +x install.src
./install.src
```

Then, you can run the program by executing:
```console

chmod +x run.src
./run.src
```
At successful execution you will see:
![Screenshot from 2023-06-20 02-30-36](https://github.com/notMuizz/ARP_Assign_03/assets/123844091/5587e10d-542d-4e5f-a221-6fd9c4bc57d4)



## User Interface 
After compiling and running the program as mentioned above, it will ask the user to select the option in konsole. In case of selecting the *normal* mode, the behavior will be identical as in Assignment 2.
![Screenshot from 2023-06-20 02-30-46](https://github.com/notMuizz/ARP_Assign_03/assets/123844091/1bef5381-6598-41e2-8c0a-372a64209bc8)


However, if the user selects the *server* mode, the program will ask them to enter the IP address and the port number of the client they want to connect to  The server is now listening and waits for the client to connect. When that happens, the two ncurses windows will be displayed and the server becomes able to be commanded by the client, showing the current position of the circle in the first window and its trajectory in the second one.
![Screenshot from 2023-06-20 02-30-57](https://github.com/notMuizz/ARP_Assign_03/assets/123844091/600a538f-7439-47d2-983b-c36c7eb4a62a)

On the other hand, if the user selects the *client* mode, the program will ask them to enter the IP address and the port number of the server they want to connect to .Before running this mode, it is required that there is a server listening and waiting for the client to connect, otherwise, it will not be possible to establish the connection, terminating the program. 
