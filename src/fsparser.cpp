/* Josh Hilliard - OpSys 3430 - Spring 2017 - Final Project */

#include <iostream> //An I/O Library  - I am unsure which I used
#include <fstream>  //An I/O Library - I am unsure which I used
#include <cstdio>   //An I/O Library - I am unsure which I used
#include <string>   //Used for Strings
#include <fcntl.h>  //Open Libary
#include <unistd.h> //Close Library
#include <array>    //Array Library

//Namespace Declarations
using namespace std;

//Struct Declarations
struct fileNameReturn{    //A Struct so I can return 2 things in fileName()
    int rootOffset;       //The offset for the next root file to be processed
    string filename;      //A String Name for the file or a command for the calling function.  May include deleted, EOR, or file83
    fileNameReturn(){     //Constructor for this struct that sets values to 0
        rootOffset = 0;
        filename = "";
    }
    void setParms(int inputINT, string inputString){  //A Function in struct that allows for easy population of the struct values
        rootOffset = inputINT;
        filename = inputString;
    }
};
struct strReturn{                         //A Struct so I can return a few datapoints from scanFileSystem()
    string strArray[514];                 //A string array used to hold each file name that is scanned in a FAT file
    int offset;                           //An int that holds the final offset from scanning each file in the FAT image
    strReturn(){                          //The constructor that does nothing
    }
    void setArray(int k, string input){   //A Function that pushes the file name as string input into a location k
        strArray[k] = input;              //Values for stArray[0] will either be the string 'error' or the number of files read
    }
    int size(){                           //A Function that returns the number of files in the filesystem.  This is kept at location 0
        return stoi(strArray[0]);
    }
    void setOffset(int i){                //This sets the final offset of the file
        offset = i;
    }
};

//File Headers
bool isFAT16(int fileID);  //Used to check if the file is of type FAT16
bool isFAT32(int fileID);  //Used to check if the file is of type FAT32
int rootOffset(int fileID);  //Used to determine the original Root File Directory offset
fileNameReturn fileName(int fileID, int offset);  //Returns a struct with the offset of the root offset and a file name or command
strReturn scanFileSystem(int fileID, int offset);  //Used to return an object that contains all files in the FAT object, as well as the final offset

//Global Variable Declarations
string deleted = "File Deleted";  //Used to determine if a file is of type deleted
string EOR = "End of Root";  //Used to determine if the end of the root directory has been reached
string file = "An unknown file or directory type";  //Temporary file name that should be deleted
string file83 = "File 8.3 Type";  //Used to determine if the 32 bype section is of a 8.3 File Name
string root = "Root File Lable";  //Used to determine if the Filename is for the root directory name
string error = "Error in Reading File";  //Used to determine if there is an error in reading the filenames

//Main Function
int main(int argc, char* argv[]){
    //Check to see if a file argument has been passed in, ends program if it
    //doesn't detect any command line arguments
    if(argc == 1){
        cout << "Please enter at least 1 FAT file location as a command line Argumet." << endl;
        return 1;
    }
  //For loop to cycle through each argument passed into the file
    for(int i = 1; i < argc; i++){
        int fileID;  //Integer file representation of a file to be opened
        fileID = open(argv[i],O_RDONLY);  //Opens a file and stores the file number into an integer
        lseek(fileID,0, SEEK_SET);  //Ensure Disk is at location 0 to start
        int offset = 0; //initilizes int variable to determine Root Offset
        offset = rootOffset(fileID);  //Finds the Root Offset from the begining of the file

      //Check to see if the file was open.  If it wasn't, display error and
      //check next file
        if(fileID == -1){
            cout << "Issues found with image \"" << argv[i] << "\"" << endl;
            cout << "File " << argv[i] << ": Cannot Open File." << endl << endl;
            continue;
        }
      //Check to see if the file can be determined to be a FAT16 or FAT32
      //file.  If it cannot, display error and continue to next file
        if(offset == -1){
            cout << "Issues found with image \"" << argv[i] << "\"" << endl;
            cout << "File " << argv[i] << ": Cannot Determine FAT16/FAT32 Status." << endl << endl;
            continue;
        }
      //scanFileSystem() determine files within an image
        strReturn leReturn = scanFileSystem(fileID, offset);
      //Begin checking for errors in the string array
      //Check to see if an error occured in reading the file directory
        if(leReturn.strArray[0] == error){
            cout << "Issues found with image \"" << argv[i] << "\"" << endl;
            cout << "File " << argv[i] << ": Cannot read a file in the root folder directory." << endl;
        }
      //Check to see if the number of files returned exceed 512, which is not
      //allowed in FAT root directories
        else if(leReturn.size() > 512){
            cout << "Issues found with image \"" << argv[i] << "\"" << endl;
            cout << "File " << argv[i] << ": Number of entries in root folder directory exceed 512." << endl;
        }
      //Check to see if the final byte offset exceeds the number of
      //hypothetical bytes allowed (512 entries, 32 bytes per entry)
        else if(leReturn.offset > 512*32){
            cout << "Issues found with image \"" << argv[i] << "\"" << endl;
            cout << "File " << argv[i] << ": Total size of root folder directory exceeds maximum possible size." << endl;
        }
      //Assume there are no errors, and print file directory
        else{
            cout << "Filesystem \"" << argv[i] << "\" has a valid root directory." << endl;
            cout << "The root folder directory listing is:" << endl;
          //Loop through each entry in the leReturn struct's array strArray
          //and print to console
            for(int j = 1; j < leReturn.size(); j++){
                if(leReturn.strArray[j] == "") continue;  //If the string is blank, it does not print to console and moves to the next value
                cout << leReturn.strArray[j] << endl;
            }
        }

        close(fileID);  //Closeing the file, just to be safe that the world doesn't explode.  Mama always told me to clean up after myself, and to not talk to strangers
        cout << endl;   //Prints a new line after each file read to help seperate output so it doesn't all mash together
    }
    return 0;  //Program ended, return 0
}

//Helper Functions
bool isFAT16(int fileID){
    int outputChar = 5;  //Set the number of chars needed
    char* bufTemp = new char[outputChar];  //Set buffer to push the read into
    char charArray[] = {'F','A','T','1','6'};  //Set a char array of what the output should be
    unsigned int* locationsHEX = new unsigned int[outputChar];  //Creates unsigned int array to store hex values of the input CHAR array
    lseek(fileID,0x36, SEEK_SET);  //Seeks disk to where FAT16 would be
    read(fileID, bufTemp, outputChar);  //Reads 5 chars at where FAT16 would be
  //Push values into locationsHEX unsigned int array, and check to see if
  //values have extra 'Fs' in the hex value.  I am unsure why some do, but
  //they do.
    for(int i = 0; i < outputChar; i++){
        locationsHEX[i] = bufTemp[i];
        if(locationsHEX[i] > 0xffffff00)locationsHEX[i]-=0xffffff00;
    }
  //Check each value to see if they match the set charArray standards.  If a
  //value does not conform, it will return false
    for(int i = 0; i < outputChar; i++){
        if((unsigned int)charArray[i] != locationsHEX[i]) return false;
    }
  //If we exit the for loop without returning a false, assume the charArray
  //matches locationsHEX array
    return true;
}

bool isFAT32(int fileID){
    int outputChar = 5;  //Set the number of chars needed
    char* bufTemp = new char[outputChar];  //Set buffer to push the read into
    char charArray[] = {'F','A','T','3','2'};  //Set a char array of what the output should be
    unsigned int* locationsHEX = new unsigned int[outputChar];  //Creates unsigned int array to store hex values of the input CHAR array
    lseek(fileID,0x52, SEEK_SET);  //Seeks disk to where FAT32 would be
    read(fileID, bufTemp, outputChar);  //Reads 5 chars at where FAT32 would be
  //Push values into locationsHEX unsigned int array, and check to see if
  //values have extra 'Fs' in the hex value.  I am unsure why some do, but
  //they do.
    for(int i = 0; i < outputChar; i++){
        locationsHEX[i] = bufTemp[i];
        if(locationsHEX[i] > 0xffffff00)locationsHEX[i]-=0xffffff00;
    }
  //Check each value to see if they match the set charArray standards.  If a
  //value does not conform, it will return false
    for(int i = 0; i < outputChar; i++){
        if((unsigned int)charArray[i] != locationsHEX[i]) return false;
    }
  //If we exit the for loop without returning a false, assume the charArray
  //matches locationsHEX array
    return true;
}

int rootOffset(int fileID){
    unsigned int reserveSectors1;  //2 bytes @ 0x0E, left byte, FAT16/32
    unsigned int reserveSectors2;
    unsigned int sectorPerFAT1;    //2 bytes @ 0x16, left byte, FAT16
    unsigned int sectorPerFAT2;    //4 bytes @ 0x24, ,FAT32
    unsigned int sectorPerFAT3 = 0;
    unsigned int sectorPerFAT4 = 0;
    unsigned int numberOfFATs;     //1 byte  @ 0x10, FAT16/32
    unsigned int bytesPerSector1;  //2 bytes @ 0x0B, right byte, FAT16/32
    unsigned int bytesPerSector2;
    char* bufTemp = new char[8];

  //Setting Reserve Sectors
    lseek(fileID,0x0e,SEEK_SET);
    read(fileID,bufTemp,2);
    reserveSectors1 = bufTemp[0];
    reserveSectors2 = bufTemp[1];
  //Setting Sectors Per FAT
  //Checks to see if file is of type FAT16, and sets hex values that are
  //needed to calculate root file offset of a FAT16 file
    if(isFAT16(fileID)){
        lseek(fileID,0x16,SEEK_SET);
        read(fileID,bufTemp,2);
        sectorPerFAT1 = bufTemp[0];
        sectorPerFAT2 = bufTemp[1];
    }
  //Checks to see if file is of type FAT32, and sets hex values that are
  //needed to calculate root file offest of a FAT32 file
    else if(isFAT32(fileID)){
        lseek(fileID,0x24,SEEK_SET);
        read(fileID,bufTemp,4);
        sectorPerFAT1 = bufTemp[0];
        sectorPerFAT2 = bufTemp[1];
        sectorPerFAT3 = bufTemp[2];
        sectorPerFAT4 = bufTemp[3];
    }
  //Unable to determine if it is FAT16 or FAT32, return an error
    else{
        return -1;
    }
  //Setting Number of FATs
    lseek(fileID,0x10,SEEK_SET);
    read(fileID,bufTemp,1);
    numberOfFATs = bufTemp[0];
  //Setting Bytes Per Sector
    lseek(fileID,0x0b,SEEK_SET);
    read(fileID,bufTemp,2);
    bytesPerSector1 = bufTemp[0];
    bytesPerSector2 = bufTemp[1];

  //Check for F's
  //This is done b/c as i was testing code, the ints would randomly get a
  //bunch of F's to preceed the hex value.  Unsure why, but this fixes it
    if(reserveSectors1 > 0xffffff00)reserveSectors1-=0xffffff00;
    if(reserveSectors2 > 0xffffff00)reserveSectors2-=0xffffff00;
    if(sectorPerFAT1 > 0xffffff00)sectorPerFAT1-=0xffffff00;
    if(sectorPerFAT2 > 0xffffff00)sectorPerFAT2-=0xffffff00;
    if(sectorPerFAT3 > 0xffffff00)sectorPerFAT1-=0xffffff00;
    if(sectorPerFAT4 > 0xffffff00)sectorPerFAT2-=0xffffff00;
    if(numberOfFATs > 0xffffff00)numberOfFATs-=0xffffff00;
    if(bytesPerSector1 > 0xffffff00)bytesPerSector1-=0xffffff00;
    if(bytesPerSector2 > 0xffffff00)bytesPerSector2-=0xffffff00;
  //Calculation of Offset
  //Checks to see if file is FAT16, and performs calculations to determine
  //Root Directory offset and returns value
    if(isFAT16(fileID)){
        return (reserveSectors1 + sectorPerFAT1 * numberOfFATs) * bytesPerSector2 * 256;
    }
  //Assumes file is of type FAT32, and perfoms calucations to determine Root
  //Directory offset and returns value
    else{
        return (reserveSectors1 + sectorPerFAT2 * numberOfFATs * 256) * bytesPerSector2 * 256;
    }
}

fileNameReturn fileName(int fileID, int offset){
    char* bufTemp = new char[32];         //Creates a buffer for each line of the Root File Directory to be held in
    unsigned int bufHex1 = 0;             //A place holder for a hex value
    unsigned int bufHex2 = 0;             //A place holder for a hex value
    int offsetTMP = 0;                    //Sets the original offset within the offset given
    int bytesRead;                        //Stores the number of bytes read in.  Used for error checking
    fileNameReturn output;                //Creates a new fileNameReturn object to be returned
    string bufString;                     //A string that is used as a buffer to hold the partial contents of a file name
    string stringContainer;               //Stores the final return string to be pushed into the fileNameReturn
    lseek(fileID,offset,SEEK_SET);        //Seeks to the offset provided to the file
    bytesRead = read(fileID,bufTemp,32);  //Reads 32 bytes into char buffer bufTemp
  //Verifies that 32 bytes were read in.  If less than 32 bytes were read in,
  //it will return an error
    if(bytesRead < 32){
        output.setParms(0,error);
        return output;
    }
    bufHex1 = bufTemp[0];                 //Stores 1st byte into int buffer bufHEX1
    bufHex2 = bufTemp[11];                //Stores 12th byte into int buffer bufHEX2
  //Checking for the plague that is the additional Fs problem noted earlier
    if(bufHex1 > 0xffffff00)bufHex1-=0xffffff00;
    if(bufHex2 > 0xffffff00)bufHex2-=0xffffff00;
  //After looking online, files with a start character of 0xE5 are a deleted
  //file, and should be skipped in file output.  Sets output to note a deleted
  //file
    if(bufHex1 == 0xE5){
        output.setParms(32,deleted);
    }
  //If a file begins with 0x00, you have reached the End of the Root
  //directory, and sets output to note End of Root
    else if(bufHex1 == 0x00){
        output.setParms(32,EOR);
    }
  //If we got this far, we assume that the file name is a valid type, so we
  //need to sift through everything to get what we want - Long File Names
    else{
      //Check to see if the 12th byte is 0x08, which is the directory
      //information for the root directory itself.  Set output to root, so
      //this can be skipped.
        if(bufHex2 == 0x08){
            output.setParms(32,root);
        }
      //Check to see if the 12th byte is 0x20, which is a listing as a 8.3
      //file type.  Set Output to file83, so this can be skipped.
        else if(bufHex2 == 0x20){
            output.setParms(32,file83);
        }
      //Check to see if the 12th byte is 0x0f, which is a component of a long
      //file name
        else if(bufHex2 == 0x0f){
          //Makes a loop that continues until I tell it not to.  This is due
          //to the variable lenght of a file name.  Note: I do not check to
          //see if file names are less than 255 characters
            while(true){
                string stringContainerTMP = "";  //Creates a temporary string
                int locations[13] = {1,3,5,7,9,14,16,18,20,22,24,28,30};  //sets the locations that a char could be in the RFD Hex
                unsigned int locationsHEX[13];  //creates a unsigned int array to hold the char values as hexes
              //Loops through the current buffer, pulling each individual char
              //into the locationsHEX array.  Checks to see if there are extra
              //Fs an removes them.  Also checks to see if the value is 0x00
              //or 0xff.  If the HEX is neither, it appends it to the
              //stringContainerTMP string
                for(int i = 0; i < 13; i++){
                    locationsHEX[i] = bufTemp[locations[i]];
                    if(locationsHEX[i] > 0xffffff00)locationsHEX[i]-=0xffffff00;
                    if(locationsHEX[i] == 0x00 || locationsHEX[i] == 0xff) break;
                    else stringContainerTMP.append(1,bufTemp[locations[i]]);
                }
                offsetTMP += 32;  //Increments the offset by 32 to check the next grouping in the RFD
                lseek(fileID,offset+offsetTMP,SEEK_SET);  //Seeks to the new location in the RFD
                read(fileID,bufTemp,32);  //Reads next 32 bytes in the RFD
                bufHex1 = bufTemp[11];  //Pushes the 12th byte into an unsigned int to be checked
                if(bufHex1 > 0xffffff00)bufHex1-=0xffffff00;  //Does the F Check
                stringContainer.insert(0,stringContainerTMP);  //Pushes current contents into the string to be pushed into output
                if(bufHex1 == 0x0f) continue;  //If the 12th byte is 0x0f, continue the loop
                else break;  //If the 12th byte is anything else, break the loop
            }
            output.setParms(offsetTMP,stringContainer);  //Set the output object to be pushed back to the calling function
        }
      //Check to see if the 12th byte is 0x10, which is a directory 8.3 file
      //name.  Set output to file83 so this can be skipped.
        else if(bufHex2 == 0x10){
            output.setParms(32,file83);
        }
      //If none of the above conditions were met for the 12th byte, I want to
      //acknowledge there is a file or directory that is an unknown type, so I
      //set output to file so it will print "An Unknown File or Directory".  I
      //was uncertain if this should be an error or not, and I opted to not
      //have it be an error, but to acknowledge there is something weird
      //happening
        else{
            output.setParms(32,file);
        }
    }
  //Returns the output object
    return output;
}

strReturn scanFileSystem(int fileID, int offset){
    fileNameReturn output;            //Creates a fileNameReturn Struct so I don't need to instanitate it a thousand times
    int fileOffset = 0;               //Creates a local file offset so I don't need to instantiate it a thousand times
    strReturn leReturn;               //Creates the object to be returned, set as leReturn, b/c I was feeling fancy at 2am when I created this
  //Cycles up to 513 times to check for filename entries in the FAT file
    for(int i =0; i < 513; i++){
            output = fileName(fileID, offset + fileOffset);  //Gets a file name
            if(output.filename == deleted);                  //Checks to see if the filename is deleted.  If it is, it continues loop
            else if(output.filename == file83);              //Checks to see if the filename is a 8.3 file name.  If it is, it continues loop
            else if(output.filename == root);                //Checks to see if the filename is a root directory information. If it is, it continues loop
          //Checks to see if the filename returns an error.  If it does, it
          //sets position 0 of the strReturn object to error, and breaks the
          //loop
            else if(output.filename == error){
                leReturn.setArray(0,error);
                break;
            }
          //Checks to see if the filename returns EOR.  If it does, it sets
          //the number of loops as the number of file entries in the RFD in
          //the strReturn object (array position 0)
            else if(output.filename == EOR){
                leReturn.setArray(0,to_string(i));
                leReturn.setOffset(fileOffset);
                break;
            }
          //If none of the above conditions were found, it adds the filename
          //to the strReturn object in the position of the current i
            else{
                leReturn.setArray(i+1, output.filename);
            }
            fileOffset += output.rootOffset;  //Adds the offset of the file(s) being processed to the current file offset
    }
    return leReturn;  //returns a strReturn object
}
