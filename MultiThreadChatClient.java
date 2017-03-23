import java.io.*;
import java.net.*;


public class MultiThreadChatClient implements Runnable {

    // The client socket
    private static Socket clientSocket = null;
    // The output stream
    private static PrintStream os = null;
    // The input stream
    private static DataInputStream is = null;

    private static BufferedReader inputLine = null;
    private static boolean closed = false;

    public static void main(String[] args) {

        // The default port.
        int portNumber = 8888;
        // The default host.
        String host = "localhost";

        if (args.length < 2) {
            System.out
                    .println("Usage: java MultiThreadChatClient <host> <portNumber>\n"
                            + "Now using host=" + host + ", portNumber=" + portNumber);
        } else {
            host = args[0];
            portNumber = Integer.valueOf(args[1]).intValue();
        }

    /*
     * Open a socket on a given host and port. Open input and output streams.
     */
        try {
            clientSocket = new Socket(host, portNumber);
            inputLine = new BufferedReader(new InputStreamReader(System.in));
            os = new PrintStream(clientSocket.getOutputStream());
            is = new DataInputStream(clientSocket.getInputStream());
        } catch (UnknownHostException e) {
            System.err.println("Don't know about host " + host);
        } catch (IOException e) {
            System.err.println("Couldn't get I/O for the connection to the host "
                    + host);
        }

    /*
     * If everything has been initialized then we want to write some data to the
     * socket we have opened a connection to on the port portNumber.
     */
        if (clientSocket != null && os != null && is != null) {
            try {

        /* Create a thread to read from the server. */
                new Thread(new MultiThreadChatClient()).start();
                while (!closed) {
                    Integer sizeOfMessage,sizeofOptions;
                    String finalVersion,commandHolder,messageHolder,optionsHolder,messageSizeHolder,optionsSizeHolder, tempHolder;
                    String userInput = inputLine.readLine();
                    commandHolder = userInput.substring(0,1);

                    switch (commandHolder) {

                        case "b":
                            messageHolder = userInput.substring(1,userInput.length());
                            for (int i = 0; i < 20; i++) {
                                commandHolder += "\0";
                            }
                            sizeOfMessage = messageHolder.length()-1;
                            messageSizeHolder= sizeOfMessage.toString();
                            finalVersion = commandHolder + messageSizeHolder;
                            for(int i = messageSizeHolder.length(); i < 8; i++){
                                finalVersion += "\0";
                            }

                            finalVersion += messageHolder;
                            System.out.println("Whats being sent to the server ---- " + finalVersion);
                            os.println(finalVersion);
                            break;

                        case "w":
                            String [] wordsHolder;

                            tempHolder = userInput.substring(1,userInput.length());
                            wordsHolder = tempHolder.split(" ");
                            if(wordsHolder[0].equals("")) {
                                optionsHolder = wordsHolder[1];
                                messageHolder = wordsHolder[2];
                            }
                            else {
                                optionsHolder = wordsHolder[0];
                                messageHolder = wordsHolder[1];
                            }
                            sizeofOptions = optionsHolder.length();
                            sizeOfMessage = messageHolder.length();


                            messageSizeHolder = sizeOfMessage.toString();
                            finalVersion = commandHolder + optionsHolder;

                            for(int i = sizeofOptions; i < 20; i++){
                                finalVersion += "\0";
                            }
                            finalVersion += sizeOfMessage.toString();
                            for(int i = messageSizeHolder.length(); i < 8; i++){
                                finalVersion += "\0";
                            }

                            finalVersion += messageHolder;



                            System.out.println("Whats being sent to the server ---- " + finalVersion);
                            os.println(finalVersion);
                            break;
                    }
                }
        /*
         * Close the output stream, close the input stream, close the socket.
         */
                os.close();
                is.close();
                clientSocket.close();
            } catch (IOException e) {
                System.err.println("IOException:  " + e);
            }
        }
    }

    /*
     * Create a thread to read from the server. (non-Javadoc)
     *
     * @see java.lang.Runnable#run()
     */
    public void run() {
    /*
     * Keep on reading from the socket till we receive "Bye" from the
     * server. Once we received that then we want to break.
     */
        String responseLine;
        String [] packet;
        try {
            while ((responseLine = is.readLine()) != null) {

                packet=responseLine.split("");

                switch (responseLine.charAt(0)){

                    case 'b':

                        System.out.println(responseLine);
                        break;

                    case 'c':
                        while ((responseLine = is.readLine()) != null){
                            if(responseLine.contains("460")){
                                System.out.println();
                                responseLine = is.readLine();
                            }
                            System.out.println(responseLine);
                            break;
                        }
                        break;

                    case 'f':
                        //Store the file onto the disk

                        break;

                    case 'w':
                        //Store username, and message into responseline then print
                        System.out.println(responseLine);
                        break;

                    default:
                        System.out.println(responseLine);
                        break;
                }



            }
            closed = true;
        } catch (IOException e) {
            System.err.println("IOException:  " + e);
        }
    }

}