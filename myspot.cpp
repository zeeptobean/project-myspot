//Default: authorization code PKCE
//otherwise use client credentials only

#include <bits/stdc++.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using json = nlohmann::json; 

#define APP_CLIENT_ID "41901dc7043b43e0a39cffab20f3b08e"
#define APP_CLIENT_SECRET "308547ce60e04601977c15753ec877b6"
#define APP_REDIRECT_URI "https://localhost:62309/callback"
#define API_ENDPOINT_STRING "https://api.spotify.com/v1/"

#define API_ENDPOINT(x) API_ENDPOINT_STRING#x

CURL *curl;
CURLcode curlres;
std::string token, refresh_token;

class image_representation {
    std::string link;
    int width, height;
    bool is_null;

    public:
    image_representation() : is_null(1), width(0), height(0) {};

    image_representation(const std::string& tlink, const int& twidth, const int& theight) {
        if(link == "") is_null = 1;
        else {
            this->link = tlink;
            this->width = twidth;
            this->height = theight;
            is_null = 0;
        }
    }

    bool get_image(std::string& tlink, int& twidth, int& theight) {
        if(is_null) return 0;
        tlink = this->link;
        twidth = this->width;
        theight = this->height;
        return 1;
    }
};

struct artist_data {
    // public:                     //temporary public for debugging
    std::string id, url, name;
    std::vector<std::string> genres;
    int64_t follower;
    int8_t popularity;
    image_representation image;

    public:
    artist_data() {};
    artist_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        url = pjson.at("external_urls").at("spotify");
        follower = pjson.at("followers").at("total");
        genres = pjson.at("genres");
        id = pjson.at("id");
        if(pjson.at("images").size() != 0) {
            const auto& firstim = pjson.at("images").front();
            image = image_representation(firstim.at("url"), firstim.at("width"), firstim.at("height"));
        }
        name = pjson.at("name");
        popularity = pjson.at("popularity");
    }

    std::string get_id() const {return id;}
    std::string get_url() const {return url;}
    std::string get_name() const {return url;}
    std::vector<std::string> get_genres() const {return genres;}
    int64_t get_follower() const {return follower;}
    int8_t get_popularity() const {return popularity;}
    image_representation get_image() const {return image;}
};

size_t onetime_post_response(char *ptr, size_t charsz, size_t strsz, void *_stdstringret) {
    *((string*)  _stdstringret) = std::string(ptr, strsz);
    return strsz;
}

std::string prepare_client_credentials_token_request() {
    return std::string("grant_type=client_credentials&client_id=") + APP_CLIENT_ID + "&client_secret=" + APP_CLIENT_SECRET;
} 

std::string prepare_authorized_header() {
    return "Authorization: Bearer " + token;
}

std::string append_api_artist_url(std::string artist_id) {
    return std::string(API_ENDPOINT(artists)) + '/' + artist_id;
}

std::string translate_curl_error(const CURLcode& code) {
    return std::string("curl failed, error ") + curl_easy_strerror(code);
}

//Return 1 if success, 0 if fail. Pass std::string pointer for more detail.
//Pointer can be null if omitted 
int get_artist_oldcall(string artist_id, artist_data& retdata, std::string *error_string) {
    CURLcode res;
    std::string response_string;
    long response_code;
    json parsed_json;

    std::string tmp = append_api_artist_url(artist_id);
    curl_easy_setopt(curl, CURLOPT_URL, tmp.c_str());
    //Prepare header
    curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(NULL, prepare_authorized_header().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_POST, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onetime_post_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        if(error_string != NULL) *error_string = translate_curl_error(res);
        return 0;
    }

    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_slist_free_all(headerlist);

    if(error_string != NULL) {
        switch(response_code) {
            case 200:
            *error_string = "HTTP OK";
            break;
            case 401:
            *error_string = "HTTP Unauthorize";
            break;
            case 403: 
            *error_string = "HTTP Bad OAuth request";
            break;
            case 404: 
            *error_string = "HTTP Resource not found";
            break;
            case 429: 
            *error_string = "HTTP Rate limit exceeded";
            break;
        }
    }
    if(response_code != 200) return 0; 
    parsed_json = json::parse(response_string);
    retdata = artist_data(parsed_json);
    return 1;
}

//POST wrapper
//@param url url to POST to. URL could have body embed
//@param header_override Override the default Content-Type: application/x-www-form-urlencoded header. 
//Pass an empty vector for no override
//@param post_field POST body. Leave empty if not needed
//@param response Response of the request
//@param error_string Pointer for error description. Pass NULL if omitted
//@return The HTTP status code. -1 if fail to make any request
int inapp_post(const std::string& url, const std::vector<std::string>& header_override, const std::string& post_field, std::string& response, std::string *error_string) {
    int response_code;
    curl_slist *headerlist = NULL;
    bool do_header_override = header_override.size() > 0 ? 1 : 0;
    bool do_write_error_string = error_string == NULL ? 0 : 1;

    if(do_header_override) {
        headerlist = curl_slist_append(NULL, header_override[0].c_str());
        for(size_t i=1; i < header_override.size(); i++) {
            headerlist = curl_slist_append(headerlist, header_override[1].c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    }
    if(post_field.size() > 0) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onetime_post_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    curlres = curl_easy_perform(curl);
    if(do_header_override) curl_slist_free_all(headerlist);
    if(curlres != CURLE_OK) {
        if(do_write_error_string) *error_string = translate_curl_error(curlres);
        return -1;
    }
    curlres = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if(do_write_error_string) {
        switch(response_code) {
            case 200:
            *error_string = "HTTP OK";
            break;
            case 400:
            *error_string = "HTTP Bad Request";
            break;
            case 401:
            *error_string = "HTTP Unauthorize";
            break;
            case 403: 
            *error_string = "HTTP Forbidden";
            break;
            case 404: 
            *error_string = "HTTP Resource not found";
            break;
            case 429: 
            *error_string = "HTTP Rate limit exceeded";
            break;
            default:
            *error_string = std::string("HTTP") + std::to_string(response_code);
            break;
        }
    }
    return response_code;
}

//Return 1 if success, 0 if fail. Pass std::string pointer for more detail.
//Pointer can be null if omitted 
int get_artist(const string& artist_id, artist_data& retdata, std::string *error_string) {
    std::string response_string;
    int postcode = inapp_post(append_api_artist_url(artist_id), {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode != 200) return 0;

    json parsed_json = json::parse(response_string);
    retdata = artist_data(parsed_json);
    return 1;
}

/*
int abnormal_client_credential_auth() {
    CURL *curl;
    CURLcode res;

    std::string postres;
    json parsed_json;

    curl = curl_easy_init();
    if(curl) {

        //Request token 
        curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
        string tmp = prepare_client_credentials_token_request();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tmp.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onetime_post_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &postres);
        res = curl_easy_perform(curl);

        parsed_json = json::parse(postres);
        token = parsed_json.at("access_token");

        char artist_id[501];
        //Request artist
        puts("enter artist's spotify id:");
        scanf("%500s", artist_id);
        
        artist_data ad;
        std::string error_string;
        if(get_artist_oldcall(curl, std::string(artist_id), ad, &error_string)) {
            puts("sus");
        } else {
            printf("function fail, error string: %s\n", error_string.c_str());
        }

        curl_easy_cleanup(curl);
    }
}
*/

////////////////////////////////////////////////////////////////

std::string prepare_code_verifier(size_t len) {
    mt19937 _rng(chrono::steady_clock::now().time_since_epoch().count());
    const std::string possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string gen;
    for(size_t i=0; i < len; i++) {
        gen += possible[uniform_int_distribution<size_t>(0,possible.length()-1)(_rng)];
    }
    return gen;
}

//Compute SHA-256
//@param str input string
//@param output byte array with at least 32 bytes 
void proceed_sha256(const std::string& str, uint8_t hash[SHA256_DIGEST_LENGTH]) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
}

std::string proceed_sha256_hashstring(const std::string& str) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    proceed_sha256(str, hash);
    char outputBuffer[65];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
    return std::string(outputBuffer);
}

//EVP_EncodeInit require alignment... would try later 
//nvm it works
std::string proceed_url_safe_base64(const string& str) {
    int expectedOutputSize = 4*((str.length()+2)/3);
    unsigned char *buf = (unsigned char*) malloc(expectedOutputSize+1);
    int outputSize = EVP_EncodeBlock(buf, (unsigned char*) str.c_str(), str.length());
    if(outputSize != expectedOutputSize) {  
        stringstream ss;
        ss << "whoops expected output size did not match actual output size, " << expectedOutputSize << " != " << outputSize;
        throw std::runtime_error(ss.str());
    }

    string tretstr = std::string((char*) buf, outputSize), retstr; 
    free(buf);

    //convert to url-safe base64: remove '=' and replace '+' and '/' by '-' and '_'
    retstr.reserve(tretstr.length());
    for(auto& c:tretstr) {
        if(c == '+') c = '-';
        else if(c == '/') c = '_';
        else if(c == '=') c = 0;
        if(c != 0) retstr.push_back(c);
    }
    return retstr;
}

std::string prepare_code_challenge(std::string codeVerifier) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    proceed_sha256(codeVerifier, hash);
    std::string hashcode ((char*) hash, 32);
    return proceed_url_safe_base64(hashcode);
}

std::string prepare_authorization_pkce(const std::string& codeVerifier, std::string& generated_state) {
    char *cptr = new char;
    mt19937 _rng(chrono::steady_clock::now().time_since_epoch().count() ^ (uint64_t) (cptr));
    std::string str = "https://accounts.spotify.com/en/authorize?response_type=code&client_id=";
    str += APP_CLIENT_ID; 
    str += "&scope=";
    str += "playlist-read-private%20playlist-read-collaborative%20user-follow-read%20user-top-read%20user-read-recently-played%20user-library-read%20user-read-email%20user-read-private";
    str += "&redirect_uri=http%3A%2F%2F127.0.0.1%3A62309%2F&state=";
    generated_state = proceed_sha256_hashstring(prepare_code_verifier(24)).substr(uniform_int_distribution(0,31)(_rng), 32);
    str += generated_state;
    str += "&code_challenge_method=S256&code_challenge=";
    str += prepare_code_challenge(codeVerifier);
    
    delete cptr;
    return str;
}

//1 if authorized, 0 if unauthorized, -1 if fail internally, -2 if state has been tampered, -3 if invalid request 
int proceed_user_auth_response(std::string res, const std::string& generated_state, std::string& code_or_error, std::string& state) {
    char *cres = (char*) calloc(res.size()+3, 1);
    char *tokptr = NULL;
    std::string resbody;

    strcpy(cres, res.c_str());
    tokptr = strtok(cres, " \n");
    //check if method is GET
    if(strcmp(tokptr, "GET") != 0) return -3;
    tokptr = strtok(NULL, " \n");
    //get body
    resbody = std::string(tokptr);
    tokptr = strtok(NULL, " \n");
    //check if not HTTP
    if(std::string(tokptr, 4) != "HTTP") return -3;
    tokptr = strtok(NULL, " \n");
    //check if "Host:"
    if(strcmp(tokptr, "Host:") != 0) return -3;
    tokptr = strtok(NULL, " \n");
    //did not check for localhost or 127.0.0.1 with port (this would be 62309)
    free(cres);

    //proceed body
    bool is_code = 1;
    std::string action[4];
    if(!(resbody[0] == '/' && resbody[1] == '?')) return -3;
    for(size_t i=2, action_sw = 0; i < resbody.size(); i++) {
        if(resbody[i] == '=' || resbody[i] == '&') action_sw++;
        else action[action_sw] += resbody[i];
    }

    if(action[2] != "state") return -3;
    if(action[0] == "error") {
        is_code = 0;
        if(action[3] != generated_state) return -2;
        code_or_error = action[1];
        state = action[3];
    }
    else if(action[0] == "code") {
        is_code = 1;
        if(action[3] != generated_state) return -2;
        code_or_error = action[1];
        state = action[3];
    } else return -3;

    return is_code;
}

#define ultra_handler(call, usermsg, morecode) if(call == -1) {\
                                fprintf(stderr, "Call %s failed: %s\n", #call, strerror(errno)); \
                                if(usermsg != NULL) fprintf(stderr, "Additional data: %s\n", usermsg); \
                                morecode();}

//simple http server to handle callback from spotify
//1 if authorized, 0 if unauthorized, -1 if fail internally, -2 if state has been tampered, -3 if invalid request 
int http_server_listen(const std::string& generated_state, std::string& code_or_error, std::string& state) {
    const char http_res_success[] = "HTTP/1.1 201 Created\r\n"
                  "Server: zeept-test local response\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<body><h2>zeept-test project</h2>"
                  "<h4>You have successfully authorized. Return to the application to continue</h4></body>\r\n";
    const char http_res_unauthorized[] = "HTTP/1.1 401 Unauthorized\r\n"
                  "Server: zeept-test local response\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<body><h2>zeept-test project</h2>"
                  "<h4>You did not accept authorization. Application will terminate</h4></body>\r\n";
    const char http_res_tampered[] = "HTTP/1.1 403 Forbidden\r\n"
                  "Server: zeept-test local response\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<body><h2>zeept-test project</h2>"
                  "<h4>An error has occurred. You should reauthenticate</h4></body>\r\n";
    const char http_res_userfail[] = "HTTP/1.1 400 Bad Request\r\n"
                  "Server: zeept-test local response\r\n"
                  "Content-type: text/html\r\n\r\n"
                  "<body><h2>zeept-test project</h2>"
                  "<h4>Bad request. You should reauthenticate</h4></body>\r\n";
    sockaddr_in sa;

    int socketfile;
    
    //retry by closing previously unclosed scoket
    for(int i=0; i < 50; i++) {
        socketfile = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(socketfile == -1) {
            perror("Fail to init socket");
            close(socketfile);
        } else break;
    }
    fprintf(stderr, "[OK] Successfully init socket\n");

        //set reusable
    int setsockopt_opt = 1;
    setsockopt(socketfile, SOL_SOCKET, SO_REUSEADDR, &setsockopt_opt, sizeof(setsockopt_opt));

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(62309);
    // sa.sin_port = 62309;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    ultra_handler(bind(socketfile, (sockaddr*) &sa, sizeof(sa)), NULL, [&socketfile](){
        close(socketfile);
        return -1;
    });
    fprintf(stderr, "[OK] socket binded\n");

    ultra_handler(listen(socketfile, 5), NULL, [&socketfile](){
        close(socketfile);
        return -1;
    });
    fprintf(stderr, "[OK] socket listening\n");

    const int tcp_read_max = 1536;
    char tcp_data[tcp_read_max+3];
    memset(tcp_data, 0, tcp_read_max+3);    //auto null-terminated
    std::string storemore;

    int connectfile = 0, readcnt = 0, writecnt = 0;

    connectfile = accept(socketfile, NULL, NULL);
    if(connectfile == -1) {
        perror("[ERROR] fail to accept connection");
        return -1;
    }
    fprintf(stderr, "[OK] connection accepted!\n");

    do {
        readcnt = read(connectfile, tcp_data, tcp_read_max);
        // fprintf(stderr, "RECEIVED DATA BEGIN\n##################\n%s################\nRECEIVED DATA END\n", tcp_data);
        if(readcnt == -1) {
            perror("[ERROR] Fail to read connection");
            close(connectfile);
            close(socketfile);
            // return -1;
        }
        storemore += std::string(tcp_data, readcnt);
        memset(tcp_data, 0, tcp_read_max+3);
    } while(readcnt == tcp_read_max);
    fputs("[OK] Done reading. Now prepare to write back\n", stderr);

    //Proceed
    int auth_ret_code = proceed_user_auth_response(storemore, generated_state, code_or_error, state);
    const char *http_resp;
    size_t http_resp_len = 0;
    switch(auth_ret_code) {
        case 1: http_resp = http_res_success; http_resp_len = sizeof(http_res_success); break;
        case 0: http_resp = http_res_unauthorized; http_resp_len = sizeof(http_res_unauthorized); break;
        case -2: http_resp = http_res_tampered; http_resp_len = sizeof(http_res_tampered); break;
        case -3: http_resp = http_res_userfail; http_resp_len = sizeof(http_res_userfail); break;
    }

    writecnt = write(connectfile, http_resp, http_resp_len);
    ultra_handler(writecnt, "fail to write back", ([&connectfile, &socketfile](){
        close(connectfile);
        close(socketfile);
        return -1;
    }));
    fprintf(stderr, "[OK] Successfully write back to user\n");

    ultra_handler(shutdown(connectfile, SHUT_RDWR), NULL, ([&connectfile, &socketfile](){
        close(connectfile);
        close(socketfile);
        return -1;
    }));
    fprintf(stderr, "[OK] Connection completed! Closing...\n");
    close(connectfile);
    close(socketfile);

    return 0;
}

std::string prepare_authorization_pkce_token_request(const std::string& auth_code, const std::string& code_verifier) {
    std::string ret = "grant_type=authorization_code&code=";
    ret += auth_code;
    ret += "&redirect_uri=http%3A%2F%2F127.0.0.1%3A62309%2F&client_id=";
    ret += APP_CLIENT_ID;
    ret += "&code_verifier=";
    ret += code_verifier;
    return ret;
}

int main() {
    std::string code_verifier = prepare_code_verifier(64);
    std::string generated_state;
    string authorization_url = prepare_authorization_pkce(code_verifier, generated_state);
    // cout << authorization_url << "\n";
    // return -1;
    int tret;

    #ifdef __linux__ 
        tret = system( (std::string("xdg-open \"") + authorization_url + '\"').c_str() );
        if(tret != 0) {
            fprintf(stderr, "Error occured, error %d!\n", tret);
            return -1;
        }
    #elif defined(_WIN32)
        CoInitialize(NULL);
        messageboxa_retry:
        if(ShellExecute(NULL, "open", authorization_url.c_str(), NULL, NULL, SW_SHOWNORMAL); < 32) {
            tret = MessageBoxA(NULL, "Operation failed", "Title", MB_RETRYCANCEL | MB_ICONERROR);
            if(tret == IDRETRY) goto messageboxa_retry;
            else if(tret = IDCANCEL) {
                CoUninitialize();
                return -1;
            }
        }
        CoUninitialize();
    #else
        #error Platform not supported
    #endif

    int retcode;
    std::string auth_code_to_xchg, ret_state;
    retcode = http_server_listen(generated_state, auth_code_to_xchg, ret_state);

    std::string postres;
    json parsed_json;

    curl = curl_easy_init();
    if(curl) {
        //Request token 
        curl_easy_setopt(curl, CURLOPT_URL, "https://accounts.spotify.com/api/token");
        string tmp = prepare_authorization_pkce_token_request(auth_code_to_xchg, code_verifier);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tmp.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onetime_post_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &postres);
        curlres = curl_easy_perform(curl);

        curlres = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);
        fprintf(stderr, "Auth ret code: %d\n", retcode);

        parsed_json = json::parse(postres);
        token = parsed_json.at("access_token");
        refresh_token = parsed_json.at("refresh_token");

        char artist_id[501];
        strcpy(artist_id, "5Pb27ujIyYb33zBqVysBkj");
        
        artist_data ad;
        std::string error_string;
        if(get_artist(std::string(artist_id), ad, &error_string)) {
            puts("sus");
        } else {
            printf("function fail, error string: %s\n", error_string.c_str());
        }

        retcode = inapp_post(API_ENDPOINT(me), {prepare_authorized_header()}, "", postres, NULL);
        parsed_json = json::parse(postres);
        std::string urname = parsed_json.at("display_name");
        printf("Ur Spotify account name: %s\n", urname.c_str());

        curl_easy_cleanup(curl);
    }

    return 0;
}