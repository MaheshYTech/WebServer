#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <cctype>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;


vector<string> SplitStr(string& strData, string delimeter);

string to_lower(const string& str ) {
    string temp = str;
    for (int i = 0; i < temp.length(); i++) {
        temp[i] = tolower(temp[i]);
    }

    return temp;
}

void to_lowercase(string& str) {
    for (char& c : str) c = std::tolower(static_cast<char>(c));
}

string getContentType(string config_file, string fileName) {
    int bytesRead = 0;
    string extn = fileName.substr(fileName.rfind("."));
    string lines;
    string contentLine = "";
    char readBuffer[1024] = { NULL };

    to_lowercase(extn);

    //main_folder + "\\Config\\content_type.txt"
    ifstream fs(config_file , ios::binary);

    while (fs.read(readBuffer, sizeof(readBuffer))) {
        lines.append(readBuffer, fs.gcount());
    }

    fs.close();

    int start = 0, end = 0;

    start = lines.find(extn.c_str());

    if (start != string::npos) {
        end = lines.find("\r\n", start);
        if (end != string::npos) {
            contentLine = lines.substr(start, end - start);
        }
    }

    if (contentLine.size() <= 0) {
        return "";
    }

    start = contentLine.find("|");
    end = contentLine.find("\r\n");

    contentLine = contentLine.substr(start + 1, end);

    return contentLine;
}

string numToString(int num) {
    string strBuff;
    stringstream ss;
    int remdrNum = 0;

    while (1) {
        remdrNum = num % 10;
        ss.str("");
        ss.clear();
        ss << remdrNum;
        strBuff = ss.str() + strBuff;
        num = num / 10;
        if (num < 10) {
            ss.str("");
            ss.clear();
            ss << num;
            strBuff = ss.str() + strBuff;
            break;
        }
    }

    return strBuff;
}

string getFileName(string qryStr) {
    string fileName = "";

    int startPos = 0;
    int endPos = 0;

    startPos = qryStr.find("GET ") + 4;
    endPos = qryStr.rfind(" HTTP/1.1");

    qryStr = qryStr.substr(startPos, endPos - startPos);
    if (qryStr.length() == 0) {
        return "/";
    }

    startPos = qryStr.rfind("/") + 1;
    fileName = qryStr.substr(startPos);
    if (fileName.length() == 0) {
        return "/";
    }

    //fileName = qryStr;

    return fileName;
}

string getFileExtn(string fileName) {
    int startPos = 0;
    int endPos = 0;

    string extn = "";

    startPos = fileName.rfind(".");

    if (startPos != string::npos) {
        extn = fileName.substr(startPos);
        //cout << fileName << " " << extn << endl;
    }

    return extn;
}

typedef struct IP_DETAILS {
    string ip_addrs;
    string ip_port;

    void update_ip_detail(SOCKADDR_IN  adrs) {
        ip_addrs = inet_ntoa(adrs.sin_addr);
        ip_port = numToString(adrs.sin_port);
    }
};

typedef struct REQT_HEADER_SETTING {
    string Sec_Fetch_Site;
    string Sec_Fetch_Mode;
    string Sec_Fetch_User;
    string Sec_Fetch_Dest;

    int ACTION_TYPE;  // 1001 ordinary process , 1002 Attachment
};

void printBanner(string bannerFile, string bannerName ) {
    stringstream ss;
    string line;
    string banner;
    string bannerStart;
    string bannerEnd;
    int len = 0;
    ifstream fs(bannerFile);
    //fs.open("D:\\Websrvr\\Config\\systems.txt");

    if (!fs.is_open()) {
        string line = "DIANA WEB SERVER";
        len = line.size();
        banner.append((120 - len) / 2, ' ');
        banner.append(line);
        cout << banner << endl;
        return;
    }

    ss << fs.rdbuf();
    bool FOUND = false;

    banner.append("\r\n\r\n");
    bannerStart.append("[]" + bannerName);
    bannerEnd.append("[/]" + bannerName);

    while (std::getline(ss, line)) {
        if (line.find(bannerStart.c_str()) != string::npos) {
            len = std::stoi(line.substr(line.rfind("-") + 1));
            len = (120 - len) / 2;
            FOUND = true;
            continue;
        }

        if (line.find(bannerEnd.c_str()) != string::npos) {
            break;
            continue;
        }

        if (FOUND) {
            banner.append(len, ' ');
            banner.append("" + line + "\r\n");
            //banner.insert(0, len, ' ');
            //cout << "";
        }
    }

    std::cout << banner << endl;

    fs.close();
}

class WEBSERV {
private:
    string base_folder;
    string content_type = "";
    REQT_HEADER_SETTING reqtHeaderSetting;

    void updateWebDir() {
        char dirBuff[256] = { NULL };


        strcpy_s(dirBuff, "D:\\Websrvr\\");
        //_getcwd(dirBuff, sizeof(dirBuff));

        base_folder = "";
        base_folder.append(dirBuff);
        base_folder.append("\\");

        //cout << base_folder << endl;
    }

    void initBaseResp() {
        httpBaseResp = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n";
    }

    string getLine(const string str , string substr) {
        string line = "";
        int start = 0, end = 0;

        if ((start = str.find(substr)) != string::npos) {
            if ((end = str.find("\r\n", start)) == string::npos) {
                return "";
            }

            return str.substr(start, end-start);
        }
        else {
            return "";
        }
    }

    string getLine(string src, string startStr, string endStr) {
        int start = src.find(startStr);
        int end = src.find(endStr, start);

        if (end == string::npos) return "";

        //string line = src.substr(start, end - start);

        return src.substr(start, end - start);
    }

    void setResponseAction() {
        string ss(buffer);
        string line = "";

        //line = getLine(ss, "Sec-Fetch-Site: ");
        line = getLine(buffer, "Sec-Fetch-Site: ", "\r\n");
        if (line.size() > 0) {
            reqtHeaderSetting.Sec_Fetch_Site = line.substr(line.find(": ") + 2);
            //sscanf(line.c_str(), "Sec-Fetch-Site: %s", reqtHeaderSetting.Sec_Fetch_Site);
        }

        //line = getLine(ss, "Sec-Fetch-Mode: ");
        line = getLine(buffer, "Sec-Fetch-Mode: ", "\r\n");
        if (line.size() > 0) {
            reqtHeaderSetting.Sec_Fetch_Mode = line.substr(line.find(": ") + 2);
            //sscanf(line.c_str(), "Sec-Fetch-Mode: %s", reqtHeaderSetting.Sec_Fetch_Mode);
        }

        //line = getLine(ss, "Sec-Fetch-User: ");
        line = getLine(buffer, "Sec-Fetch-User: ", "\r\n");
        if (line.size() > 0) {
            reqtHeaderSetting.Sec_Fetch_User = line.substr(line.find(": ") + 2);
            //sscanf(line.c_str(), "Sec-Fetch-User: %s", reqtHeaderSetting.Sec_Fetch_User);
        }

        //line = getLine(ss, "Sec-Fetch-Dest: ");
        line = getLine(buffer, "Sec-Fetch-Dest: ", "\r\n");
        if (line.size() > 0) {
            reqtHeaderSetting.Sec_Fetch_Dest = line.substr(line.find(": ") + 2);
            //sscanf(line.c_str(), "Sec-Fetch-Dest: %s", reqtHeaderSetting.Sec_Fetch_Dest);
        }

        if (reqtHeaderSetting.Sec_Fetch_Site == "same-origin" &&
            reqtHeaderSetting.Sec_Fetch_Mode == "navigate" &&
            reqtHeaderSetting.Sec_Fetch_User == "?1" &&
            reqtHeaderSetting.Sec_Fetch_Dest == "document") 
        {
            reqtHeaderSetting.ACTION_TYPE = 1002;
        }
        else {
            reqtHeaderSetting.ACTION_TYPE = 1001;
        }
        
        //std::begin(ReqVect.begin(), "");
    }

    void initProcess() {
        updateWebDir();
    }

    void updateContentType(string fileName) {
        string config_file = base_folder + "\\Config\\content_type.txt";
        content_type = getContentType(config_file, fileName);
    }

    void print_baner() {
        string tempStr;
        tempStr.append(base_folder);
        tempStr.append("\\Config\\Banner.txt");

        printBanner(tempStr, "DIYANA");
        printBanner(tempStr, "WEBSERVER");
        cout << endl << endl;
    }

public:
    string httpReqStr;
    string httpBaseResp;
    string httpResp;
    string file_path;
    string reqt_type;
    string physical_folder;
    string file_type;
    string file_content;
    string pres_req_type;
    char folder_path[255];
    char file_name[64];
    char buffer[4096];
    int LISTEN_PORT;
    vector<string> ReqVect;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;

    WEBSERV(){
        physical_folder = base_folder;
        initProcess();
        
        print_baner();
        cout << endl;
    }

    WEBSERV(int port) {
        physical_folder = base_folder;
        LISTEN_PORT = port;
        initProcess();

        print_baner();
    }

    string initServerSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            return "INVALID_SOCKET";
        }
        return "OK";
    }

    void bindAndListen() {
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
        serverAddr.sin_port = htons(LISTEN_PORT);

        bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        cout << "Listening on Port " << LISTEN_PORT << endl;
        listen(serverSocket, SOMAXCONN);

        clientComunicate();
    }

    void clientComunicate() {
        int clientSize;
        while (true) {
            IP_DETAILS ip_detail;
            clientSize = sizeof(clientAddr);

            clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

            if (clientSocket == INVALID_SOCKET) {
                continue;
            }

            memset(buffer, 0, sizeof(buffer));

            int byteRcvd = recv(clientSocket, buffer, sizeof(buffer), 0);


            if (strlen(buffer) <= 0) {
                continue;
            }

            IN_ADDR client_ip = clientAddr.sin_addr;

            updateReqTable(buffer, "\r\n");

            reqt_type = getReqtType(ReqVect[0]);
            if (reqt_type != "POST") {
                procesGetReq(ReqVect[0]);
            }

            //updateContentType(file_name);

            ip_detail.update_ip_detail(clientAddr);


            cout << "*****************************************************************" << endl;
            cout << "Client Connected IP :: PORT   ========> " <<  ip_detail.ip_addrs ;
            cout <<  " :: " << ip_detail.ip_port << endl;
            cout << ReqVect[0] << endl;
            cout << "*****************************************************************" << endl;

            std::cout << endl << endl;

            initBaseResp();


            setResponseAction();
            
            if (reqt_type == "POST") {
                if ((string(buffer)).find("boundary=") != string::npos) {
                    postProcessFormData();
                }
                else {
                    postProcessNormalData();
                }

            }else {
                procesGetFileDownload(file_type);
                //if (reqtHeaderSetting.ACTION_TYPE == 1002) {
                //    procesGetFileDownload(file_type);
                //}
                //else {
                //    procesGetFile(file_type);
                //}
            }
            
            closesocket(clientSocket);
        }

        closesocket(serverSocket);
    }

    string getReqtType(string reqtStr) {
        int endPos = reqtStr.find(" ");
        return reqtStr.substr(0, endPos);
    }

    int CheckDirectoryExist(string dirPath) {
        struct stat info;
        int result = stat(dirPath.c_str(), &info);

        if (result != 0) {
            return 0;
        }
        else if (info.st_mode & S_IFDIR) {
            return 1;
        }
        else {
            return 2;
        }
    }

    int getStrPos(const char* str) {
        int i = 0;
        while (1) {
            if (str[i] == '\r' && str[i+1] == '\n') {
                break;
            }
            i++;
        }

        return i;
    }

    void postProcessNormalData() {
        string post_data = string(buffer).substr((string(buffer)).rfind("\r\n\r\n")+4);
        vector<string> postVect;
        int start = 0;
        int end = 0;
        while (1) {
            end = post_data.find("&" , start);
            if (end == string::npos) { 
                postVect.push_back(post_data.substr(start));
                break; 
            }
            postVect.push_back(post_data.substr(start, end - start));
            start = end + 1;
        }

        string msg = u8"Uploaded Data Saved Successfully";
        int content_len = msg.length();

        httpBaseResp = "HTTP/1.1 200 OK\r\n";
        httpBaseResp = httpBaseResp + "Content-Type: text/plain; charset=utf-8\r\n";
        httpBaseResp = httpBaseResp + "Content-Length: " + std::to_string(content_len) + "\r\n";
        httpBaseResp = httpBaseResp + "Connection: close\r\n";
        httpBaseResp = httpBaseResp + "\r\n";
        httpBaseResp = httpBaseResp + msg;
        
        sendRespHeaderToClient();

        cout << post_data << endl;
    }

    void postProcessFormData() {
        char* boudryLine = strstr(buffer, "boundary=");
        int byteRcvd = 0;
        int startPos = 0;
        int endPos = 0;
        string boundaryStr;
        string dataUploaded;
        string endBoundary;
        string rootDir = "";
        bool SAVE_UPLOAD = false;

        rootDir.append(buffer, getStrPos(buffer) | 0);
        startPos = rootDir.find("POST /") + 6;
        endPos = rootDir.rfind(" HTTP/1.1");
        rootDir = rootDir.substr(startPos, endPos - startPos);

        rootDir = rootDir.substr(0, rootDir.find("/"));

        startPos = 0;
        endPos = 0;

        dataUploaded.append(buffer);
        startPos = dataUploaded.find("boundary=") + string("boundary=").length();
        endPos = dataUploaded.find("\r\n", startPos);
        boundaryStr = dataUploaded.substr(startPos, endPos - startPos);
        endBoundary = "--" + boundaryStr + "--";

        if (boudryLine != NULL) {
            //***************************************************************
            //*****Read Data From Data Uploaded and Append to buffer********* 
            //***************************************************************
            while (1) {
                if (dataUploaded.rfind(endBoundary) == string::npos) {
                    byteRcvd = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (byteRcvd <= 0) {  // End of Received Data From Client
                        httpBaseResp = "HTTP/1.1 200 OK\r\n";
                        httpBaseResp += "Connection: close\r\n";
                        httpBaseResp += "\r\n";
                        sendRespHeaderToClient();
                        closesocket(clientSocket);
                        return;
                        //break;
                    }
                }

                dataUploaded.append(buffer, byteRcvd);
                int ival = 0;

                ival = dataUploaded.find(endBoundary);
                if (ival >= 0) break;
            }


            dataUploaded = dataUploaded.substr(dataUploaded.find("\r\n\r\n") + 4);

            int fileNameStart = 0;
            int fieldNameStart = 0;
            int file_index = 1;
            string contentLine;
            string saveDir = "";


            boundaryStr = "--" + boundaryStr;
            cout << endl << endl << endl;

            vector<string>  vectFieldNames;
            while (1) {
                startPos = dataUploaded.find(boundaryStr);
                
                if (startPos < 0) break;

                if (dataUploaded.find(endBoundary) == 0) {
                    break;
                }
                startPos = dataUploaded.find("Content-Disposition:", startPos);
                endPos = dataUploaded.find("\r\n", startPos);
                contentLine = dataUploaded.substr(startPos, endPos - startPos); // .find("filename=");
                fileNameStart = contentLine.find("filename=");

                if (fileNameStart > 0) {   // File Uploaded Part
                    fileNameStart += 10;
                    string fileToSave = contentLine.substr(fileNameStart, (contentLine.length() - fileNameStart) - 1);
                    if (fileToSave.length() <= 0) {
                        fileToSave.append("file_" + numToString(file_index++) + ".dat");
                    }

                    dataUploaded = dataUploaded.substr(endPos);
                    startPos = dataUploaded.find("\r\n\r\n") + 4;
                    endPos = dataUploaded.find("\r\n" + boundaryStr);
                    
                    saveDir = base_folder  ;
                    saveDir.append(rootDir + "//Download//");

                    if (!CheckDirectoryExist(saveDir.c_str()))
                    {
                        int status = _mkdir(saveDir.c_str());
                        //cout << status << endl;
                    }

                    fileToSave = saveDir + fileToSave;
                    std::ofstream outFile(fileToSave, std::ios::binary);
                    outFile << dataUploaded.substr(startPos, endPos - startPos);
                    outFile.flush();
                    outFile.close();
                    cout << fileToSave << " File Has Been Saved , Total Bytes " << (endPos - startPos)  << endl;
                    dataUploaded = dataUploaded.substr(endPos);
                }
                else {    // Form Data Uploaded Part
                    string fieldName = "";
                    string fieldValue = "";

                    fieldNameStart = contentLine.find("name=") + 6;
                    endPos = contentLine.find("\r\n");
                    if (endPos <= 0) endPos = contentLine.length() - 1;
                    fieldName = contentLine.substr(fieldNameStart, endPos - fieldNameStart);

                    dataUploaded = dataUploaded.substr(contentLine.length());
                    startPos = dataUploaded.find("\r\n\r\n") + 4;
                    endPos = dataUploaded.find("\r\n", startPos);
                    fieldValue = dataUploaded.substr(startPos, endPos - startPos);
                    vectFieldNames.push_back(fieldName + " - " + fieldValue);
                    dataUploaded = dataUploaded.substr(endPos + 2);
                    cout << dataUploaded << endl;
                }
            }
            SAVE_UPLOAD = true;
        }
        //"Content-Length: " +  file_size_str.str()  +  "\r\n";
        int content_len= 0;

        string msg = u8"Uploaded Data Saved Successfully";
        if (SAVE_UPLOAD) {    
            content_len = msg.length();
            httpBaseResp = "HTTP/1.1 200 OK\r\n"; 
            httpBaseResp = httpBaseResp + "Content-Type: text/plain; charset=utf-8\r\n";
            httpBaseResp = httpBaseResp + "Content-Length: " + std::to_string(content_len)  + "\r\n"; 
            httpBaseResp = httpBaseResp + "Connection: close\r\n" ;
            httpBaseResp = httpBaseResp + "\r\n";
            httpBaseResp = httpBaseResp + msg;
        }
        else {
            msg = "Upload Data Failed";
            content_len = msg.length();
            httpBaseResp = "HTTP/1.1 200 OK\r\n";
            httpBaseResp = httpBaseResp + "Content-Type: text/plain\r\n";
            httpBaseResp = httpBaseResp + "Content-Length: " + std::to_string(content_len) + "\r\n";
            httpBaseResp = httpBaseResp + "Connection: Close\r\n";
            httpBaseResp = httpBaseResp + "\r\n";
            httpBaseResp = httpBaseResp + msg;
        }

        sendRespHeaderToClient();
    }

    void sendRespHeaderToClient() {
        try {
            send(clientSocket, httpBaseResp.c_str(), httpBaseResp.length(), 0);
        }
        catch (exception ex) {
            cout << ex.what() << endl;
        }
    }

    void updateReqTable(string reqStr , string strDel) {
        ReqVect.clear();
        ReqVect = SplitStr(reqStr , strDel);
    }

    void getReqType(string reqType) {
        int pos = 0;
        pos = reqType.find("GET ", 0);
        if (pos == string::npos) {
            pres_req_type = "GET";
        }

        pos = reqType.find("POST ", 0);
        
        if (pos == string::npos) {
            pres_req_type = "POST";
        }

        pres_req_type =  "";
    }

    void procesGetReq(string reqStr) {
        int start = 4;
        int end = reqStr.find(" HTTP/1.1");
        string getData = reqStr.substr(4, end-4);
        string file_name = "";
        string file_extn = "";

        file_path = getData;

        file_name = getFileName(reqStr);
        file_extn = getFileExtn(file_name);

        updateContentType(file_name);

        if ((end = getData.find("?", 0)) != string::npos) {
            file_type = "Query";
        }else if (file_name.length() > 1 ) {
            file_type = file_extn; // getData.substr(end);
            cout <<  "File Type Is :: :: "  <<  file_type << endl;
        }else {
            file_type = ".html";
            if (getData.length() == 1) {
                file_path.append("index.html");
            }
            else {
                file_path.append("index.html");
            }
        }

        cout << getData << endl;
    }

    void procesGetFileDownload(string reqStr) {
        string read_file = "";
        string file_size_str;
        ifstream fs;

        updateContentType(file_path);

        if (reqtHeaderSetting.ACTION_TYPE == 1002) { 
            //************  Get Request 
            //************  Http Request Click on Link 
            //************  <a href="" /a>
            int pos = 0, file_size = 0;
            string file_name_download = "";
            read_file = base_folder + file_path;



            fs.open(read_file.c_str(), ios::binary);

            if (!fs.is_open()) {
                httpBaseResp = "HTTP / 1.1 401 OK\r\n";
                httpBaseResp = httpBaseResp + "Connection: close\r\n";
                httpBaseResp = httpBaseResp + "\r\n";
                send(clientSocket, httpBaseResp.c_str(), httpBaseResp.size(), 0);
                return;
            }

            fs.seekg(0, ios::end);
            file_size = fs.tellg();

            pos = file_path.find_last_of("/\\");
            file_name_download = file_path.substr(pos + 1);

            file_size_str = std::to_string(file_size);

            if (content_type.size() == 0) {
                content_type = "application/octet-stream";
            }

            //*********Sending HTTP Response Header************
            httpBaseResp = "HTTP/1.1 200 OK\r\n";
            //httpBaseResp = httpBaseResp + "Content-Type: application/octet-stream\r\n";
            httpBaseResp = httpBaseResp + "Content-Type: " + content_type  + "\r\n";
            httpBaseResp = httpBaseResp + "Content-Length: " + file_size_str + "\r\n";
            httpBaseResp = httpBaseResp + "Accept-Ranges: bytes\r\n";
            httpBaseResp = httpBaseResp + "Content-Disposition: attachment; fileName = " + file_name_download + "\r\n";
            httpBaseResp = httpBaseResp + "Connection: Alive\r\n\r\n";

            //************** Sending Data ********************
            send(clientSocket, httpBaseResp.c_str(), strlen(httpBaseResp.c_str()), 0);

            //*********Sending File Content************
            sendBinaryFile(fs);

            fs.close();
            cout << "*** XLSX **** " << endl;
        }
        else if (reqtHeaderSetting.ACTION_TYPE == 1001) {  // Normal Http Request
            //************  Get Request 
            //************  <link> , <scipt> <img> <video>
            
            if (reqtHeaderSetting.Sec_Fetch_Dest == "video") {
                sendVideoFile();
            }
            else {
                sendGetReqtFile();
            }
        }
    }

    void sendGetReqtFile() {
        ifstream fs;
        string read_file;
        int file_size = 0;
        read_file = base_folder + file_path;

        //**************************************************
        //************ File Open For Read ******************
        //**************************************************
        fs.open(read_file.c_str(), ios::binary);

        if (fs.bad()) { //fp == NULL
            httpBaseResp = "HTTP/1.1 200 OK\r\n";
            httpBaseResp = httpBaseResp + "Connection: Close\r\n\r\n";
            send(clientSocket, httpBaseResp.c_str(), strlen(httpBaseResp.c_str()), 0);
            return;
        }

        fs.seekg(0, ios::end);
        file_size = fs.tellg();

        //file_size_str << file_size;

        //*********Sending HTTP Response Header************
        httpBaseResp = "HTTP/1.1 200 OK\r\n";
        httpBaseResp = httpBaseResp + "Content-Type: " + content_type  + "\r\n";
        httpBaseResp = httpBaseResp + "Content-Length: " + std::to_string(file_size)  + "\r\n";
        httpBaseResp = httpBaseResp + "Accept - Ranges: bytes \r\n";
        httpBaseResp = httpBaseResp + "Connection: Close\r\n\r\n";
        send(clientSocket, httpBaseResp.c_str(), strlen(httpBaseResp.c_str()), 0);

        //*********Sending File Content************
        sendBinaryFile(fs);

        cout << "*** JPEG **** " << endl;
    }



    void sendVideoFile() {
        int startPos = 0;
        int endPos = 0;
        int file_size;
        int CHUNK_SIZE = 1024 * 1024;
        long startByte = 0;
        long endByte = 0;
        char* vidBuffer;
        string strBuffer = "";
        string read_file;

        strBuffer = getByteRange(buffer);
        sscanf(strBuffer.c_str(), "Range: bytes=%zu-%zu", &startByte, &endByte);

        read_file = base_folder + file_path;

        ifstream vfs(read_file, ios::binary | ios::ate);
        file_size = vfs.tellg();
        vfs.seekg(0, ios::beg);

        endByte = (startByte + CHUNK_SIZE) >= file_size ? file_size - 1 : (startByte + CHUNK_SIZE);

        int byte_to_send = endByte - startByte + 1;

        ostringstream os;

        os << "HTTP/1.1 206 Partial Content\r\n";
        os << "Content-Type: video/mp4\r\n";
        os << "Accept-Ranges: bytes\r\n";
        os << "Content-Length: " << byte_to_send << "\r\n";
        os << "Content-Range: bytes ";
        os << startByte << "-" << endByte << "/" << file_size << "\r\n";
        os << "Connection: close" << "\r\n\r\n";

        httpBaseResp = os.str();
        send(clientSocket, httpBaseResp.c_str(), httpBaseResp.size(), 0);

        vidBuffer = new char[CHUNK_SIZE];

        vfs.seekg(startByte, ios::beg);
        int data_read = 0;
        while (byte_to_send > 0) {
            vfs.read(vidBuffer, CHUNK_SIZE);
            data_read = vfs.gcount();
            cout << "Byte To Send :: " << byte_to_send << " Data Read :: " << data_read << endl;
            if (data_read <= 0) {
                httpBaseResp = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
                send(clientSocket, httpBaseResp.c_str(), httpBaseResp.size(), 0);
                break;
            }
            send(clientSocket, vidBuffer, data_read, 0);
            byte_to_send -= data_read;
        }

        if (endByte + 1 >= file_size) {
            httpBaseResp = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
            send(clientSocket, httpBaseResp.c_str(), httpBaseResp.size(), 0);
        }

        vfs.close();
    }

    void procesGetFile(string get_file_type) {
        string read_file = "";
        std::stringstream file_size_str;
        int file_size = 0;
        ifstream fs;

       string tmp_file_type =  to_lower(get_file_type);
       if (tmp_file_type == ".cfg") {
           string temp_file_path = base_folder + file_path;
           FILE* fp = fopen(temp_file_path.c_str() , "rt");
           char line[80];
           bool found = false;
           while (fgets(line, 80, fp)) {
               string strLine = line;
               if (strLine.find("download") != string::npos) {
                   strLine = strLine.substr(strLine.find("=")+1);
                   strLine = strLine.substr(0, strLine.find('\n'));
                   physical_folder = base_folder + strLine + "//";
                   found = true;
                   break;
               }
           }

           if (!found) {
               physical_folder = base_folder + "Download//";
           }
           fclose(fp);
       }    
    }

    string getByteRange(const char *reqHeader)
    {
        string str_header = reqHeader;
        int spos = str_header.find("Range: bytes=");
        int epos = str_header.find("\r\n", spos);

        if (epos > 0) {
            str_header = str_header.substr(spos, epos);
        }
        else {
            str_header = str_header.substr(spos);
        }

        return str_header;
    }

    void sendBinaryFile(ifstream& fs) {
        fs.seekg(0, ios::beg);

        char* readBuffer = new char[1024 * 10];

        int count = 0;
        int byteSent = 0;

        while (1) {
           fs.read(readBuffer, 1024 * 10);
           count = fs.gcount();
           if (count <= 0) {
               break;
           }
           else {
               byteSent = send(clientSocket, readBuffer, count, 0);
           }
        }
    }

};

vector<string> SplitStr(string& strData, string delimeter) {
    vector<string> strVect;
    string strTok;
    int start = 0, end;

    while (1) {
        end = strData.find(delimeter, start);
        if (end == string::npos) {
            break;
        }
        strVect.push_back(strData.substr(start, end - start));
        start = end + delimeter.length();
    }

    return strVect;
}


int main()
{
    WEBSERV wsrv =  WEBSERV(8080);
    WSADATA wsData;

    //SOCKET serverSocket, clientSocket;
    //sockaddr_in serverAddr, clientAddr;
    //int  clientSize;

    WSAStartup(MAKEWORD(2, 2), &wsData);

    if (wsrv.initServerSocket()  == "INVALID_SOCKET") {
        std:cerr << "Socket Creation Failed " << endl;
        WSACleanup();
        return 0;
    }

    wsrv.bindAndListen();

    WSACleanup();
    _getch();

    return 0;
}
