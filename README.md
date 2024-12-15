# bashshell

This is a simple Linux Shell application which allows the user to perform various built in tasks whilst also allowing the execution of external system commands where none have been coded in the application.
This code program is designed to take in Command from the user and carry them out using the fork command in Linux or using built in Function to perform some functions.

Also maintains a history of the user's last 20 commands they can later invoke and a small set of aliased commands may also be set by the user and later invoked to perform their aliased function.

# How to run
## Windows
1. Ensure that Cygwin is installed, and has the gcc compiler.
2. Clone the repo.
3. Run Cygwin, and navigate to your cloned repo.
4. Compile the bashshell.c file using the following command: gcc bashshell.c -o bashshell.exe
5. Run bashshell.exe

## Linux
1. Clone the repo.
2. Compile the bashshell.c file using the following command: gcc bashshell.c -o bashshell
3. Run the compiled file using the following command: ./bashshell
