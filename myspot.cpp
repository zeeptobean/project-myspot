//Default: authorization code PKCE
//otherwise use client credentials only
//market availablity not implemented

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

//If the width or/and height is 0, it means spotify didn't supply that parameter
class image_representation {
    std::string link;
    int width, height;
    bool is_null;

    public:
    image_representation() : width(0), height(0), is_null(1) {};

    image_representation(const std::string& tlink, const int& twidth, const int& theight) {
        if(link == "") is_null = 1;
        else {
            this->link = tlink;
            this->width = twidth;
            this->height = theight;
            is_null = 0;
        }
    }

    //return 0 if image is null
    bool get_image(std::string& tlink, int& twidth, int& theight) const {
        if(is_null) return 0;
        tlink = this->link;
        twidth = this->width;
        theight = this->height;
        return 1;
    }
};

class multipage_query {
    std::string response_url_next_page, response_url_prev_page, response_url_cur_page;
    int32_t response_limit, response_offset;

    public:
    multipage_query() : response_limit(0), response_offset(0) {};
    multipage_query(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        response_url_cur_page = pjson.at("href");
        response_url_prev_page = pjson.at("previous").is_null() ? "" : pjson.at("previous");
        response_url_next_page = pjson.at("next").is_null() ? "" : pjson.at("next");
        response_offset = pjson.at("offset");
        response_limit = pjson.at("limit");
    }

    //return 0 if fail, 1 if success and internal state modified
    int load_prev_tracks_page(json& return_json) {

    }

    //return 0 if fail, 1 if success and internal state modified
    int load_next_tracks_page(json& return_json) {
        
    }

    //return 0 if fail, 1 if success and internal state modified
    int load_current_tracks_page(json& return_json) {
        
    }
};

//Would not implement audio analysis, too much data
//Audio feature would be integrated into track data for ease of computation
class audio_feature_data {
    private:
    std::string id;
    long long duration_ms; 
    bool is_major;      //major is 1, minor is 0
    int key, time_signature;
    float tempo, loudness;
    //all the reason why i made this piece of code
    float acousticness, danceability, energy, instrumentalness, liveness, speechiness, valence;

    std::string af_track_id_from_href(std::string href) {
        return href.substr(href.find_last_of('/'));
    }

    public:
    audio_feature_data() {};
    audio_feature_data(const json& pjson, const std::string& track_id = "") {
        set_data(pjson, track_id);
    };

    void set_data(const json& pjson, const std::string& track_id = "") {
        id = track_id == "" ? af_track_id_from_href(pjson.at("track_href")) : track_id;
        duration_ms = pjson.at("duration_ms");
        is_major = pjson.at("mode");
        key = pjson.at("key");
        time_signature = pjson.at("time_signature");
        tempo = pjson.at("tempo");
        loudness = pjson.at("loudness");
        acousticness = pjson.at("acousticness");
        danceability = pjson.at("danceability");
        energy = pjson.at("energy");
        instrumentalness = pjson.at("instrumentalness");
        liveness = pjson.at("liveness");
        speechiness = pjson.at("speechiness");
        valence = pjson.at("valence");
    }

    std::string get_track_key() {
        const std::string audio_feature_key_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        std::string keystr, modestr;
        modestr = is_major ? " Major" : " Minor";
        keystr = key == -1 ? "Undetected" : audio_feature_key_name[key];  //can be branchless
        return keystr + modestr;
    }
};

class simplified_artist_data {
    protected:
    std::string id, url, name;

    public:
    simplified_artist_data() {};
    simplified_artist_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        url = pjson.at("external_urls").at("spotify");
        id = pjson.at("id");
        name = pjson.at("name");
    }

    const std::string& get_id() const {return id;}
    const std::string& get_url() const {return url;}
    const std::string& get_name() const {return name;}
};

class artist_data : public simplified_artist_data {
    std::vector<std::string> genres;
    long long follower;         //long long to suppress System-V's int64_t defined as sign long, gerenate warning to scanf/printf
    int32_t popularity;
    image_representation image;

    public:
    artist_data() {};
    artist_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        simplified_artist_data::set_data(pjson);
        follower = pjson.at("followers").at("total");
        genres = pjson.at("genres");
        id = pjson.at("id");
        if(pjson.at("images").size() != 0) {
            const auto& firstim = pjson.at("images").front();
            image = image_representation(firstim.at("url"), firstim.at("width"), firstim.at("height"));
        }
    }

    const std::vector<std::string>& get_genres() const {return genres;}
    const long long& get_follower() const {return follower;}
    const int32_t& get_popularity() const {return popularity;}
    const image_representation& get_image() const {return image;}
};

class simplified_track_data {
    protected:
    std::string url, id, name;
    std::vector<std::string> available_markets;
    std::vector<simplified_artist_data> artists;
    int32_t disc_number, track_number;
    long long duration_ms;
    bool is_explicit;
    std::string restrictions, preview_url;
    audio_feature_data audio_feature;

    public:
    simplified_track_data() {};
    simplified_track_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        url = pjson.at("external_urls").at("spotify");
        id = pjson.at("id");
        name = pjson.at("name");
        if(pjson.contains("available_markets")) {
            available_markets = pjson.at("available_markets");
        }
        disc_number = pjson.at("disc_number");
        track_number = pjson.at("track_number");
        restrictions = pjson.at("restrictions").at("reason");
        preview_url = pjson.at("preview_url").is_null() ? "" : pjson.at("preview_url");
        is_explicit = pjson.at("explicit");
        duration_ms = pjson.at("duration_ms");
        for(auto& ele:pjson.at("artists")) {
            artists.push_back(simplified_artist_data(ele));
        }
    }

    int query_audio_feature() {

    }

    const std::string& get_id() const {return id;}
    const std::string& get_url() const {return url;}
    const std::string& get_name() const {return name;}
    const std::vector<std::string>& get_available_markets() const {return available_markets;}
    const int32_t& get_disc_number() const {return disc_number;}
    const int32_t& get_track_number() const {return track_number;}
    const long long& get_duration_ms() const {return duration_ms;}
    const bool& get_is_explicit() const {return is_explicit;}
    const std::string& get_restrictions() const {return restrictions;}
    const std::string& get_preview_url() const {return preview_url;}
    const audio_feature_data& get_audio_feature() const {return audio_feature;}
};

class track_data : public simplified_track_data {
    private:
    std::string isrc_id, ean_id, upc_id;
    int32_t popularity;
    simplified_album_data album;

    public:
    track_data() {};
    track_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        simplified_track_data::set_data(pjson);
        album.set_data(pjson.at("album"));
        popularity = pjson.at("popularity");
        isrc_id = pjson.at("external_ids").contains("isrc") ? pjson.at("external_ids").at("isrc") : "";
        ean_id = pjson.at("external_ids").contains("ean") ? pjson.at("external_ids").at("ean") : "";
        upc_id = pjson.at("external_ids").contains("upc") ? pjson.at("external_ids").at("upc") : "";
    }

    const std::string& get_isrc() const {return isrc_id;}
    const std::string& get_ean() const {return ean_id;}
    const std::string& get_upc() const {return upc_id;}
    const int32_t& get_popularity() const {return popularity;}
    const simplified_album_data& get_album_appear_on() const {return album;}
};

class simplified_album_data {
    protected:
    std::string id, url, name, album_type;
    int32_t total_tracks;
    std::vector<std::string> available_markets;
    std::vector<simplified_artist_data> artists;
    std::string release_date, release_date_precision, restrictions;
    image_representation image;

    public:
    simplified_album_data() {};
    simplified_album_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        album_type = pjson.at("album_type");
        url = pjson.at("external_urls").at("spotify");
        id = pjson.at("id");
        name = pjson.at("name");
        total_tracks = pjson.at("total_tracks");
        if(pjson.contains("available_markets")) {
            available_markets = pjson.at("available_markets");
        }
        if(pjson.at("images").size() != 0) {
            const auto& firstim = pjson.at("images").front();
            image = image_representation(firstim.at("url"), firstim.at("width"), firstim.at("height"));
        }
        release_date = pjson.at("release_date");
        release_date_precision = pjson.at("release_date_precision");
        restrictions = pjson.at("restrictions").at("reason");
        for(auto& ele:pjson.at("artists")) {
            artists.push_back(simplified_artist_data(ele));
        }
    }

    const std::string& get_id() const {return id;}
    const std::string& get_url() const {return url;}
    const std::string& get_name() const {return name;}
    const std::string& get_album_type() const {return album_type;}
    const int32_t& get_total_tracks() const {return total_tracks;}
    const std::vector<std::string>& get_available_markets() const {return available_markets;}
    const std::vector<simplified_artist_data> get_artists() const {return artists;};
    const std::string& get_restrictions() const {return restrictions;}
    const image_representation& get_image() const {return image;}
    const std::string& get_release_date() const {return release_date;}   //fix later
};

class album_data : public simplified_album_data {
    private:
    std::vector<simplified_track_data> items;
    std::string label;
    std::string isrc_id, ean_id, upc_id;
    std::vector<std::string> copyrights;
    std::vector<std::string> genres;
    int32_t popularity;

    public:
    album_data() {};
    album_data(const json& pjson) {
        set_data(pjson);
    };

    void set_data(const json& pjson) {
        simplified_album_data::set_data(pjson);
        label = pjson.at("label");
        isrc_id = pjson.at("external_ids").contains("isrc") ? pjson.at("external_ids").at("isrc") : "";
        ean_id = pjson.at("external_ids").contains("ean") ? pjson.at("external_ids").at("ean") : "";
        upc_id = pjson.at("external_ids").contains("upc") ? pjson.at("external_ids").at("upc") : "";
        popularity = pjson.at("popularity");
        genres = pjson.at("genres");
        for(auto& ele:pjson.at("copyrights")) {
            char tbuf[10];
            memset(tbuf, 0, 10);
            sprintf(tbuf, "(%s): ", std::string(ele.at("type")).c_str());
            copyrights.push_back(std::string(tbuf) + std::string(ele.at("text")));
        }

        for(auto& ele:pjson.at("tracks").at("items")) {
            items.push_back(simplified_track_data(ele));
        }
    }

    const std::string& get_label() {return label;}
    const std::string& get_isrc() const {return isrc_id;}
    const std::string& get_ean() const {return ean_id;}
    const std::string& get_upc() const {return upc_id;}
    const std::vector<std::string>& get_copyrights() const {return copyrights;}
    const std::vector<std::string>& get_genres() const {return genres;}
    const int32_t& get_popularity() const {return popularity;}
};

class user_data {
    private:
    std::string name, id, url;
    long long follower;
    //spotify do not guarentee largest image come first so get all
    std::vector<image_representation> image_vec;

    public:
    user_data() = default;
    user_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        name = pjson.at("display_name").is_null() ? "" : pjson.at("display_name");
        url = pjson.at("external_urls").at("spotify");
        id = pjson.at("id");
        follower = pjson.at("followers").at("total");
        for(auto& ele:pjson.at("images")) {
            image_vec.push_back(image_representation(ele.at("url"), ele.at("width"), ele.at("height")));
        }
        sort(image_vec.begin(), image_vec.end(), [](const image_representation& a, const image_representation& b) {
            //welp shit api design
            int aw, ah, bw, bh;
            std::string st;
            a.get_image(st, aw, ah);
            b.get_image(st, bw, bh);
            return (long long) aw*ah > (long long) bw*bh;
        });
    }

    const std::string& get_id() const {return id;}
    const std::string& get_url() const {return url;}
    const std::string& get_name() const {return name;}
    const long long& get_follower() const {return follower;}
    const std::vector<image_representation>& get_images() const {return image_vec;}
};

class private_user_data : public user_data {
    private:
    std::string country, email, plan;
    bool explicit_filter_on;

    public:
    private_user_data() = default;
    private_user_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        user_data::set_data(pjson);
        country = pjson.at("country");
        email = pjson.at("email");
        plan = pjson.at("product");
        explicit_filter_on = pjson.at("explicit_content").at("filter_enabled");
    }

    const std::string& get_country() const {return country;}
    const std::string& get_email() const {return email;}
    const std::string& get_plan() const {return plan;}
    const bool& get_explicit_filter_on() const {return explicit_filter_on;}
};

class simplified_playlist_data {
    bool is_collab, is_public;
    std::string description, snapshot_id;
    std::string name, url, id;
    image_representation image;
    user_data owner;
    long long tracks_count;

    public:
    simplified_playlist_data() = default;
    simplified_playlist_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        is_collab = pjson.at("collaborative");
        is_public = pjson.at("public");
        description = pjson.at("description");
        snapshot_id = pjson.at("snapshot_id");
        name = pjson.at("display_name");
        url = pjson.at("external_urls").at("spotify");
        id = pjson.at("id");
        tracks_count = pjson.at("tracks").at("total");
        owner.set_data(pjson.at("owner"));
        if(pjson.at("images").size() != 0) {
            const auto& firstim = pjson.at("images").front();
            image = image_representation(firstim.at("url"), firstim.at("width"), firstim.at("height"));
        }
    }
};

class playlist_data : public simplified_playlist_data {
    long long follower;
    std::vector<playlist_track_data> items;

    public: 
    playlist_data() = default;
    playlist_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        simplified_playlist_data::set_data(pjson);
        follower = pjson.at("followers").at("total");
        for(auto& ele : pjson.at("tracks").at("items")) {
            items.push_back(playlist_track_data(ele));
        }
    }
};

class playlist_track_data {
    std::string added_time;
    user_data added_user;
    track_data item;
    bool is_track;

    public:
    playlist_track_data() = default;
    playlist_track_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        added_time = pjson.at("added_at");
        added_user = user_data(pjson.at("added_by"));
        if(pjson.at("track").at("type") == "track") {
            is_track = 1;
            item = track_data(pjson.at("track"));
        } else is_track = 0;
    }
};

class category_data {
    vector<image_representation> image_vec;
    std::string id, name, url;      //url is api url

    public:
    category_data() = default;
    category_data(const json& pjson) {
        set_data(pjson);
    }

    void set_data(const json& pjson) {
        for(auto& ele:pjson.at("images")) {
            image_vec.push_back(image_representation(ele.at("url"), ele.at("width"), ele.at("height")));
        }
        sort(image_vec.begin(), image_vec.end(), [](const image_representation& a, const image_representation& b) {
            //welp shit api design
            int aw, ah, bw, bh;
            std::string st;
            a.get_image(st, aw, ah);
            b.get_image(st, bw, bh);
            return (long long) aw*ah > (long long) bw*bh;
        });
        id = pjson.at("id");
        name = pjson.at("name");
        url = pjson.at("href");
    }

    const vector<image_representation>& get_image() const {return image_vec;}
    const std::string& get_id() const {return id;}
    const std::string& get_url() const {return url;}
    const std::string& get_name() const {return name;}
};

//Implement podcast & show data so we might filter those out ... or we wouldn't 
class podcast_data {};

class episode_data {};

class audiobook_data {};

//easier management?
//no more function for this struct welp
struct recommendations_query_data {
    long long min_duration_ms, max_duration_ms, target_duration_ms;
    int min_key, max_key, target_key;       //Only target_key is useful. Suggestedly set all three values the same
    int min_mode, max_mode, target_mode;       //Again just like key, either 0 or 1, and all three values the same
    int min_time_signature, max_time_signature, target_time_signature;  //Again just like key, wtf
    int min_popularity, max_popularity, target_popularity;
    int min_tempo, max_tempo, target_tempo;
    float min_acousticness, max_acousticness, target_acousticness;
    float min_danceability, max_danceability, target_danceability;
    float min_energy, max_energy, target_energy;
    float min_instrumentalness, max_instrumentalness, target_instrumentalness;
    float min_liveness, max_liveness, target_liveness;
    float min_loudness, max_loudness, target_loudness;      //ew loudness war again?
    float min_speechiness, max_speechiness, target_speechiness;      
    float min_valence, max_valence, target_valence;      

    recommendations_query_data() {
        //Default init
        //floating-point value would be NaN
        //integer value would be -1
        memset(this, -1, sizeof(*this));
    };
}


size_t onetime_post_response(char *ptr, size_t charsz, size_t strsz, void *_stdstringret) {
    (void) charsz;  //set unused
    *((string*)  _stdstringret) = std::string(ptr, strsz);
    return strsz;
}

std::string prepare_client_credentials_token_request() {
    return std::string("grant_type=client_credentials&client_id=") + APP_CLIENT_ID + "&client_secret=" + APP_CLIENT_SECRET;
} 

std::string prepare_authorized_header() {
    return "Authorization: Bearer " + token;
}

std::string append_url_field(std::string url, const std::vector<std::pair<std::string, std::string>>& field) {
    url.push_back('?');
    for(auto& ele:field) {
        std::string firststr, secondstr;
        for(auto& c:ele.first) {
            if(isspace(c)) {    //change whitespace to +
                c = '+';
            } else if(isalnum(c)) {
                firststr += c;
            } else {
                char tbuf[10];
                sprintf(tbuf, "%%%02x", c);
                firststr += std::string(tbuf);
            }
        }
        for(auto& c:ele.second) {
            if(!isalnum(c)) {
                char tbuf[10];
                sprintf(tbuf, "%%%02x", c);
                secondstr += std::string(tbuf);
            } else secondstr += c;
        }
        url = url + ele.first + '=' + ele.second + '&';
    }
    url.pop_back();     //pop extra amperstand
    return url;
}

std::string translate_curl_error(const CURLcode& code) {
    return std::string("curl failed, error ") + curl_easy_strerror(code);
}

//GET annd POST wrapper
//@param post_or_get 1 for post, 0 for get
//@param url url to POST to. URL could have body embed
//@param header_override Override the default Content-Type: application/x-www-form-urlencoded header. 
//Pass an empty vector for no override
//@param post_field POST body. Leave empty if not needed
//@param response Response of the request
//@param error_string Pointer for error description. Pass NULL if omitted
//@return The HTTP status code. -1 if fail to make any request
int inapp_post_or_get(int post_or_get, const std::string& url, const std::vector<std::string>& header_override, const std::string& post_field, std::string& response, std::string *error_string) {
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
    curl_easy_setopt(curl, CURLOPT_POST, post_or_get);
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

int inapp_post(const std::string& url, const std::vector<std::string>& header_override, const std::string& post_field, std::string& response, std::string *error_string) {
    return inapp_post_or_get(1, url, header_override, post_field, response, error_string);
}

int inapp_get(const std::string& url, const std::vector<std::string>& header_override, const std::string& post_field, std::string& response, std::string *error_string) {
    return inapp_post_or_get(0, url, header_override, post_field, response, error_string);
}

//Return 1 if success, -1 if fail. 0 if token expired and should reauthenticate
//Pass std::string pointer for more detail. Pointer can be null if omitted 
int get_artist(const string& artist_id, artist_data& retdata, std::string *error_string) {
    std::string response_string;
    std::string artists_api_url = API_ENDPOINT(artists) + '/' + artist_id;
    int postcode = inapp_get(artists_api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        retdata = artist_data(parsed_json);
        return 1;
    }
    return -1;
}

//maximum 50 elements!
int get_artists(const std::vector<std::string>& artists_id, std::vector<artist_data>& retvec, std::string *error_string) {
    std::string response_string;
    std::string artists_api_url = API_ENDPOINT(artists) + '?';
    if(artists_id.size() == 0) {
        if(error_string != NULL) *error_string = "artists_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:artists_id) {
        (artists_api_url += str).append("%2C");
    }
    int postcode = inapp_get(artists_api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        json parsed_json = json::parse(response_string).at("artists");
        for(auto& ele:parsed_json) {
            retvec.push_back(artist_data(ele));
        }
        return 1;
    }
    return -1;
}

//@param artist_id The request artist id
//@param include_groups If not specified, all groups are included
//@param market unused
//@param limit Return limit. Min 0, max 50
//@param offset The index of first item to return
//@param retalbum vector of returned albums
//@param artist_relation_to_album Artist relation to returned album
//@param next_query Data structure for next index page if needed
//@param error_string Pass C++ string pointer for extra error message
//@return 1 if success, -1 if fail. 0 if token expired and should reauthenticate
int get_artist_albums(const std::string& artist_id, const std::string& include_groups, const std::string& market, const int& limit, const int& offset, std::vector<simplified_album_data>& retalbum, std::vector<std::string>& artist_relation_to_album, multipage_query& next_query, std::string *error_string) {
    (void) market;
    std::string response_string;
    std::string api_url = API_ENDPOINT(artists) + '/' + artist_id + '/' + "albums";
    api_url = append_url_field(api_url, [&include_groups, &limit, &offset]() -> auto {
        std::vector<std::pair<std::string, std::string>> fieldvec {
            std::make_pair("limit", std::to_string(limit)),
            std::make_pair("offset", std::to_string(offset))
        };
        if(include_groups.length() != 0) fieldvec.push_back(std::make_pair("include_groups", include_groups));
        return fieldvec;
    }());
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query.set_data(parsed_json);
        retalbum.clear();
        artist_relation_to_album.clear();
        for(auto& ele:parsed_json.at("items")) {
            retalbum.push_back(simplified_album_data(ele));
            artist_relation_to_album.push_back(ele.at("album_group"));
        }
        return 1;
    }
    return -1;
}

int get_artist_top_tracks(const std::string& artist_id, const std::string& market, std::vector<track_data>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(artists) + '/' + artist_id + '/' + "top-tracks";
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        json parsed_json = json::parse(response_string).at("tracks");
        for(auto& ele:parsed_json) {
            retvec.push_back(track_data(ele));
        }
        return 1;
    }
    return -1;
}

int get_artist_related_artists(const std::string& artist_id, const std::string& market, std::vector<artist_data>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(artists) + '/' + artist_id + '/' + "related-artists";
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        json parsed_json = json::parse(response_string).at("artists");
        for(auto& ele:parsed_json) {
            retvec.push_back(artist_data(ele));
        }
        return 1;
    }
    return -1;
}

int get_user_genre_seeds(std::vector<std::string>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(recommendations/available-genre-seeds);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec = json::parse(response_string).at("genres");
        return 1;
    }
    return -1;
}

//maximum 50 elements!
int get_tracks(const std::vector<std::string>& tracks_id, const std::string& market, std::vector<track_data>& retvec, std::string *error_string) {
    (void) market;  //let market unused for now
    std::string response_string;
    std::string api_url = API_ENDPOINT(tracks) + '?';
    if(tracks_id.size() == 0) {
        if(error_string != NULL) *error_string = "tracks_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:tracks_id) {
        (api_url += str).append("%2C");
    }
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        json parsed_json = json::parse(response_string).at("tracks");
        for(auto& ele:parsed_json) {
            retvec.push_back(track_data(ele));
        }
        return 1;
    }
    return -1;
}

int get_track(const std::string& track_id, const std::string& market, track_data& retdata, std::string *error_string) {
    std::vector<track_data> retvec;
    int tret = get_tracks({track_id}, market, retvec, error_string);
    retdata = retvec[0];
    return tret;
}

int get_user_saved_tracks(const std::string& market, const int& limit, const int& offset, std::vector<track_data>& retvec, std::vector<std::string>& timestringvec, multipage_query& next_query, std::string *error_string) {
    (void) market;  //let market unused for name
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/tracks);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        std::make_pair("limit", std::to_string(limit)), std::make_pair("offset", std::to_string(offset))
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query.set_data(parsed_json);
        retvec.clear();
        timestringvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(track_data(ele.at("track")));
            timestringvec.push_back(ele.at("added_at"));
        }
        return 1;
    }
    return -1;
}

//ew
int check_tracks_saved(const std::vector<std::string>& tracks_id, std::vector<bool>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/tracks/contains) + '?';
    if(tracks_id.size() == 0) {
        if(error_string != NULL) *error_string = "tracks_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:tracks_id) {
        (api_url += str).append("%2C");
    }
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        retvec = (std::vector<bool>) json::parse(response_string);
        return 1;
    }
    return -1;
}

int get_tracks_feature(const std::vector<std::string>& tracks_id, std::vector<audio_feature_data>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(audio_features) + '?';
    if(tracks_id.size() == 0) {
        if(error_string != NULL) *error_string = "tracks_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:tracks_id) {
        (api_url += str).append("%2C");
    }
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        json parsed_json = json::parse(response_string).at("audio_features");
        //wouldn't care if a track's id is invalid cuz this is not a public api yet  
        for(size_t i=0; i < tracks_id.size(); i++) {
            retvec.push_back(audio_feature_data(parsed_json[i], tracks_id[i]));
        }
        return 1;
    }
    return -1;
}

int get_track_feature(const std::string& track_id, audio_feature_data& retdata, std::string *error_string) {
    std::vector<audio_feature_data> retvec;
    int tret = get_tracks_feature({track_id}, retvec, error_string);
    retdata = retvec[0];
    return tret;
}

int get_user_recommendations(const int& limit, const std::string& market, const std::vector<std::string>& seed_artists, const std::vector<std::string>& seed_genres, const std::vector<std::string>& seed_tracks, const recommendations_query_data& rcm, std::vector<track_data>& retvec, std::string *error_string) {
    
}

int get_current_user_profile(private_user_data& userdata, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json pjson = json::parse(response_string);
        userdata = private_user_data(pjson);
        return 1;
    }
    return -1;
}

int get_user_profile(const std::string& user_id, user_data& userdata, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(users) + '/' + user_id;
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json pjson = json::parse(response_string);
        userdata = user_data(pjson);
        return 1;
    }
    return -1;
}

int get_current_user_top_tracks(const std::string& time_range_str, const int& limit, const int& offset, std::vector<track_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/top/tracks);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("time_range", time_range_str),
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query.set_data(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(track_data(ele));
        }
    }
    return -1;
}

int get_current_user_top_artists(const std::string& time_range_str, const int& limit, const int& offset, std::vector<artist_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/top/artists);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("time_range", time_range_str),
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query.set_data(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(artist_data(ele));
        }
    }
    return -1;
}

//ew
int get_followed_artists(const std::string& request_id_after, const int& limit, std::vector<artist_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/following);
}

//ew
int check_followed_artists(const std::vector<std::string>& artists_id, std::vector<bool>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/following/contains?type=artist&);
    if(artists_id.size() == 0) {
        if(error_string != NULL) *error_string = "artists_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:artists_id) {
        (api_url += str).append("%2C");
    }
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        retvec = (std::vector<bool>) json::parse(response_string);
        return 1;
    }
    return -1;
}

//ew
int check_followed_users(const std::vector<std::string>& users_id, std::vector<bool>& retvec, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/following/contains?type=user&);
    if(users_id.size() == 0) {
        if(error_string != NULL) *error_string = "users_id vector can't be empty";
        return -1;
    }
    //do have trailing comma
    for(auto& str:users_id) {
        (api_url += str).append("%2C");
    }
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        retvec.clear();
        retvec = (std::vector<bool>) json::parse(response_string);
        return 1;
    }
    return -1;
}

//Check if current user following a playlist. ew
//@param playlist_id Playlist id
//@param error_string Pass C++ string pointer for extra error message 
//@return -1 if fail. 0 if token expired and should reauthenticate. 10 if return true and -10 for false
int check_followed_playlist(const std::string& playlist_id, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(playlists/) + playlist_id + "/followers/contains";
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        return (bool) json::parse(response_string) ? 10 : -10;
    }
    return -1;
}

int get_playlist(const std::string& playlist_id, const std::string& market, const std::string& field, playlist_data& retdata, multipage_query& next_query, std::string *error_string) {
    (void) market;  (void) field;
    std::string response_string;
    std::string api_url = API_ENDPOINT(playlists/) + playlist_id;
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        retdata.set_data(parsed_json);
        next_query = multipage_query(parsed_json.at("tracks"));
        return 1
    }
    return -1;
}

int get_playlist_items(const std::string& playlist_id, const std::string& market, const std::string& field, const int& limit, const int& offset, std::vector<playlist_track_data>& retvec, multipage_query& next_query, std::string *error_string) {
    (void) market;  (void) field;
    std::string response_string;
    std::string api_url = API_ENDPOINT(playlists/) + playlist_id + "/tracks";
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(playlist_track_data(ele));
        }
        return 1;
    }
    return -1;
}

int get_current_user_playlists(const int& limit, const int& offset, std::vector<simplified_playlist_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(me/playlists);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        std::make_pair("limit", std::to_string(limit)), std::make_pair("offset", std::to_string(offset))
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(simplified_playlist_data(ele));
        }
        return 1;
    }
    return -1;
}

int get_user_playlists(const std::string& user_id, const int& limit, const int& offset, std::vector<simplified_playlist_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(users/) + user_id + "/playlists";
    std::vector<std::pair<std::string, std::string>> fieldvec {
        std::make_pair("limit", std::to_string(limit)), std::make_pair("offset", std::to_string(offset))
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(simplified_playlist_data(ele));
        }
        return 1;
    }
    return -1;
}

//Get featured playlists
//@param locale_str an ISO 639-1 language code and an ISO 3166-1 alpha-2 country code.
//If empty supplied, it would be American English (en_US)
//@param limit
//@param offset
//@param retvec
//@param localized_message
//@param next_query Data structure for next index page if needed
//@param error_string Pass C++ string pointer for extra error message
//@return 1 if success, -1 if fail. 0 if token expired and should reauthenticate
int get_featured_playlists(const std::string& locale_str, const int& limit, const int& offset, std::vector<simplified_playlist_data>& retvec, std::string& localized_message, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(browse/featured-playlists);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("locale", locale_str);
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        localized_message = parsed_json.at("message");
        parsed_json = parsed_json.at("playlists");
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(simplified_playlist_data(ele));
        }
        return 1;
    }
    return -1;
}

//Ez version for any string
int get_category_playlists_ez(const std::string& category_str, const int& limit, const int& offset, std::vector<simplified_playlist_data>& retvec, std::string& localized_message, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(browse/categories/) + category_str + "/playlists";
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        localized_message = parsed_json.at("message");
        parsed_json = parsed_json.at("playlists");
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(simplified_playlist_data(ele));
        }
        return 1;
    }
    return -1;
}

//Strict version require properly fetched category object
int get_category_playlists(const category_data& category, const int& limit, const int& offset, std::vector<simplified_playlist_data>& retvec, multipage_query& next_query, std::string *error_string) {
    return get_category_playlists_ez(category.get_name(), limit, offset, retvec, next_query, error_string);
}

//ew
int get_playlist_cover_image(const std::string& playlist_id, image_representation& retdata, std::string *error_string) {

}

int get_multiple_categories(const std::string& locale_str, const int& limit, const int& offset, std::vector<category_data>& retvec, multipage_query& next_query, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(browse/categories);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("locale", locale_str);
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        parsed_json = parsed_json.at("categories");
        next_query = multipage_query(parsed_json);
        retvec.clear();
        for(auto& ele:parsed_json.at("items")) {
            retvec.push_back(category_data(ele));
        }
        return 1;
    }
    return -1;
}

//What is the point of this api when you don't know the right category id?
//@return -1 if fail. 0 if user should reauthorize. 10 if category exists. -10 if not
int get_category(const std::string& category_id, const std::string& locale_str, category_data& retdata, std::string *error_string) {
    std::string response_string;
    std::string api_url = API_ENDPOINT(browse/categories/) + category_id + "?locale=" + locale_str;
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        retdata.set_data(parsed_json);
        return 10;
    } else if(postcode == 404) {
        memset(&retdata, 0, sizeof(category_data));     //side-effect lul
        return -10;
    }
    return -1;
}

//ew
int get_available_market(std::vector<std::string>& retvec, std::string *error_string) {

}

//no podcast, show, audiobook support
//bad design, but easiest to work with
int search_for_item(const std::string& query_str, const std::vector<std::string>& query_type, const std::string& market, const int& limit, const int& offset, std::vector<simplified_track_data>& tracks_vec, std::vector<simplified_artist_data>& artists_vec, std::vector<simplified_album_data>& albums_vec, std::vector<simplified_playlist_data>& playlists_vec, multipage_query& track_next_query, multipage_query& artist_next_query, multipage_query& album_next_query, multipage_query& playlist_next_query, std::string *error_string) {
    (void) market;
    std::string response_string;
    std::string api_url = API_ENDPOINT(search/);
    std::vector<std::pair<std::string, std::string>> fieldvec {
        make_pair("q", query_str);
        make_pair("limit", std::to_string(limit)),
        make_pair("offset", std::to_string(offset)),
    };
    api_url = append_url_field(api_url, fieldvec);
    int postcode = inapp_get(api_url, {prepare_authorized_header()}, "", response_string, error_string);
    if(postcode == 401) return 0;
    else if(postcode == 200) {
        json parsed_json = json::parse(response_string);
        
        track_next_query = multipage_query(parsed_json.at("tracks"));
        tracks_vec.clear();
        for(auto& ele:parsed_json.at("tracks").at("items")) {
            tracks_vec.push_back(simplified_track_data(ele));
        }
        artist_next_query = multipage_query(parsed_json.at("artists"));
        artists_vec.clear();
        for(auto& ele:parsed_json.at("artists").at("items")) {
            artists_vec.push_back(simplified_artist_data(ele));
        }
        album_next_query = multipage_query(parsed_json.at("albums"));
        albums_vec.clear();
        for(auto& ele:parsed_json.at("albums").at("items")) {
            albums_vec.push_back(simplified_album_data(ele));
        }
        playlist_next_query = multipage_query(parsed_json.at("playlists"));
        playlists_vec.clear();
        for(auto& ele:parsed_json.at("playlists").at("items")) {
            playlists_vec.push_back(simplified_playlist_data(ele));
        }
        
    }
    return -1;
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
    int expectedOutputSize = 4*int((str.length()+2)/3);
    unsigned char *buf = (unsigned char*) malloc(expectedOutputSize+1);
    int outputSize = EVP_EncodeBlock(buf, (const unsigned char*) str.c_str(), (int) str.length());
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

    const size_t tcp_read_max = 1536;
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
        readcnt = (int) read(connectfile, tcp_data, tcp_read_max);
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
    const char *http_resp = NULL;
    size_t http_resp_len = 0;
    switch(auth_ret_code) {
        case 1: http_resp = http_res_success; http_resp_len = sizeof(http_res_success); break;
        case 0: http_resp = http_res_unauthorized; http_resp_len = sizeof(http_res_unauthorized); break;
        case -2: http_resp = http_res_tampered; http_resp_len = sizeof(http_res_tampered); break;
        case -3: http_resp = http_res_userfail; http_resp_len = sizeof(http_res_userfail); break;
    }

    writecnt = (int) write(connectfile, http_resp, http_resp_len);
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

    return auth_ret_code;
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
    std::string error_string;

    int tret;
    std::string postres;
    json parsed_json;

    curl = curl_easy_init();

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

    std::string auth_code_to_xchg, ret_state;
    tret = http_server_listen(generated_state, auth_code_to_xchg, ret_state);
    if(tret == -1 || tret == -3) fprintf(stderr, "[ERROR] HTTP server listen fail. Panic!\n");
    else if(tret == 0) fprintf(stderr, "[INFO] User did not authorized. Terminating\n");
    else if(tret == -2) fprintf(stderr, "[ERROR] Authorization state has been tampered! Panic!\n");
    else if(tret == 1) {
        if(curl) {
            //Request token 
            string tmp = prepare_authorization_pkce_token_request(auth_code_to_xchg, code_verifier);
            tret = inapp_post("https://accounts.spotify.com/api/token", {}, tmp.c_str(), postres, &error_string);
            if(tret == 200) {
                parsed_json = json::parse(postres);
                token = parsed_json.at("access_token");
                refresh_token = parsed_json.at("refresh_token");
                fprintf(stderr, "[OK] Tokens obtained!\n");

                char artist_id[501];
                strcpy(artist_id, "1vCWHaC5f2uS3yhpwWbIA6");
                
                //artist data example: Avicii
                artist_data ad;
                if(get_artist(std::string(artist_id), ad, &error_string) == 1) {
                    fprintf(stderr, "Artist name: %s. Follower: %lld. Popularity: %d\nGenres: ", ad.get_name().c_str(), ad.get_follower(), ad.get_popularity());
                    for(auto& s:ad.get_genres()) fprintf(stderr, "%s, ", s.c_str());
                    fprintf(stderr, "\n[INFO] END Aviici profile example\n");
                } else {
                    fprintf(stderr, "get_artist failed, error string: %s\n", error_string.c_str());
                }

                tret = inapp_get(API_ENDPOINT(me), {prepare_authorized_header()}, "", postres, NULL);
                parsed_json = json::parse(postres);
                std::string urname = parsed_json.at("display_name");
                printf("Ur Spotify account name: %s\n", urname.c_str());
            } else {
                fprintf(stderr, "[ERROR] Fail to request token (user authorized). Err %s\n", error_string.c_str());
            }

            curl_easy_cleanup(curl);
        } else {
            fputs("[ERROR] cURL init failed. Panic!\n", stderr);
        }
    }

    return 0;
}