#include "myspot.cpp"

void test_self() {
    std::string error_string;

    printf("\nBEGIN self test\n");

    private_user_data myself;
    if(get_current_user_profile(myself, NULL) == 1) {
        printf("Username: %s\nFollower: %lld\nCountry: %s\nPlan: %s\n", myself.get_name().c_str(), myself.get_follower(), myself.get_country().c_str(), myself.get_plan().c_str());
    } else printf("get_current_user_profile failed\n");

    std::vector<std::string> user_genre_seed;
    if(get_user_genre_seeds(user_genre_seed, NULL) == 1) {
        puts("Your genre seeds:");
        for(auto& sgs:user_genre_seed) printf("%s, ", sgs.c_str());
        puts("");
    } else printf("get_user_genre_seeds failed\n");

    multipage_query next_query;
    std::vector<track_data> track_vec;
    if(get_current_user_top_tracks("short_term", 5, 0, track_vec, next_query, NULL) == 1) {
        puts("\nYour short-term top tracks:");
        int counter = 1;
        for(auto& td:track_vec) {
            printf("%d. ", counter);
            for(auto& ats:td.get_artists()) {
                printf("%s, ", ats.get_name().c_str());
            }
            printf("- %s\n", td.get_name().c_str());
            printf("Popularity %d. From %s: %s. Released %s\n", td.get_popularity(), td.get_album_appear_on().get_album_type().c_str(), td.get_album_appear_on().get_name().c_str(), td.get_album_appear_on().get_release_date().c_str());
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    if(get_current_user_top_tracks("medium_term", 5, 0, track_vec, next_query, NULL) == 1) {
        puts("Your medium-term top tracks:");
        int counter = 1;
        for(auto& td:track_vec) {
            printf("%d. ", counter);
            for(auto& ats:td.get_artists()) {
                printf("%s, ", ats.get_name().c_str());
            }
            printf("- %s\n", td.get_name().c_str());
            printf("Popularity %d. From %s: %s. Released %s\n", td.get_popularity(), td.get_album_appear_on().get_album_type().c_str(), td.get_album_appear_on().get_name().c_str(), td.get_album_appear_on().get_release_date().c_str());
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    if(get_current_user_top_tracks("long_term", 5, 0, track_vec, next_query, NULL) == 1) {
        puts("Your long-term top tracks:");
        int counter = 1;
        for(auto& td:track_vec) {
            printf("%d. ", counter);
            for(auto& ats:td.get_artists()) {
                printf("%s, ", ats.get_name().c_str());
            }
            printf("- %s\n", td.get_name().c_str());
            printf("Popularity %d. From %s: %s. Released %s\n", td.get_popularity(), td.get_album_appear_on().get_album_type().c_str(), td.get_album_appear_on().get_name().c_str(), td.get_album_appear_on().get_release_date().c_str());
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    std::vector<artist_data> artist_vec;
    if(get_current_user_top_artists("short_term", 5, 0, artist_vec, next_query, NULL) == 1) {
        puts("Your short-term top artists:");
        int counter = 1;
        for(auto& ad:artist_vec) {
            printf("%d. %s. Follower: %lld. Popularity: %d\nGenres: ", counter, ad.get_name().c_str(), ad.get_follower(), ad.get_popularity());
            for(auto& s:ad.get_genres()) printf("%s, ", s.c_str());
            puts("");
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    if(get_current_user_top_artists("medium_term", 5, 0, artist_vec, next_query, NULL) == 1) {
        puts("Your medium-term top artists:");
        int counter = 1;
        for(auto& ad:artist_vec) {
            printf("%d. %s. Follower: %lld. Popularity: %d\nGenres: ", counter, ad.get_name().c_str(), ad.get_follower(), ad.get_popularity());
            for(auto& s:ad.get_genres()) printf("%s, ", s.c_str());
            puts("");
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    if(get_current_user_top_artists("long_term", 5, 0, artist_vec, next_query, NULL) == 1) {
        puts("Your long-term top artists:");
        int counter = 1;
        for(auto& ad:artist_vec) {
            printf("%d. %s. Follower: %lld. Popularity: %d\nGenres: ", counter, ad.get_name().c_str(), ad.get_follower(), ad.get_popularity());
            for(auto& s:ad.get_genres()) printf("%s, ", s.c_str());
            puts("");
            counter++;
        }
        puts("");
    } else printf("get_current_user_top_tracks failed\n");

    std::vector<std::string> string_vec;
    if(get_user_saved_tracks("", 7, 1, track_vec, string_vec, next_query, NULL) == 1) {
        puts("Some of your saved tracks");
        for(auto& td:track_vec) {
            int counter = 0;
            printf("Track: ");
            for(auto& ats:td.get_artists()) {
                printf("%s, ", ats.get_name().c_str());
            }
            printf("- %s\n", td.get_name().c_str());
            printf("Popularity %d. From %s: %s. Saved at %s\n", td.get_popularity(), td.get_album_appear_on().get_album_type().c_str(), td.get_album_appear_on().get_name().c_str(), string_vec[counter].c_str());
            counter++;
        }
        puts("");
    } else printf("get_user_saved_tracks failed");

    std::vector<bool> boolvec;    
    if(check_tracks_saved({"5odlY52u43F5BjByhxg7wg","5eTaQYBE1yrActixMAeLcZ","2aEuA8PSqLa17Y4hKPj5rr"}, boolvec, &error_string) == 1) {
        puts("Check if you saved these tracks...");
        printf("JVKE - golden hour: %s\n", boolvec[0] ? "yes" : "no");
        printf("Calvin Harris & Ellie Goulding - Miracle: %s\n", boolvec[1] ? "yes" : "no");
        printf("Modern Talking - Cheri Cheri Lady: %s\n", boolvec[2] ? "yes" : "no");
        puts("");
    } else printf("check_tracks_saved failed. Error string: %s\n", error_string.c_str());

    printf("\nEND self test\n");
}

void test_avicii(std::string artist_id = "1vCWHaC5f2uS3yhpwWbIA6") {
    std::string error_string;
    multipage_query mq;

    printf("\nBEGIN Avicii test\n");

    artist_data ad;
    if(get_artist(artist_id, ad, &error_string) == 1) {
        printf("Artist name: %s. Follower: %lld. Popularity: %d\nGenres: ", ad.get_name().c_str(), ad.get_follower(), ad.get_popularity());
        for(auto& s:ad.get_genres()) printf("%s, ", s.c_str());
        puts("");
    } else {
        printf("get_artist failed, error string: %s\n", error_string.c_str());
    }

    std::vector<simplified_album_data> simple_album_vec;
    std::vector<std::string> artist_relation_album_vec;
    if(get_artist_albums(artist_id, "album,single,appears_on", "", 10, 0, simple_album_vec, artist_relation_album_vec, mq, &error_string) == 1) {
        puts("Top 10 Avicii album/single work:");
        for(int i=0; i < 10; i++) {
            printf("No %d. %s: %s. Released %s\n", i+1, artist_relation_album_vec[i].c_str(), simple_album_vec[i].get_name().c_str(), simple_album_vec[i].get_release_date().c_str());
        }
        puts("");
    } else {
        printf("get_artist_albums failed, error string: %s\n", error_string.c_str());
    }

    std::vector<track_data> track_vec;
    std::vector<audio_feature_data> track_af_data;
    if(get_artist_top_tracks(artist_id, "", track_vec, &error_string) == 1) {
        if(get_tracks_feature([&track_vec]() {
            std::vector<std::string> tvec;      tvec.reserve(25);
            for(auto& tv:track_vec) tvec.push_back(tv.get_id());
            return tvec;
        }(), track_af_data, NULL) != 1) {
            printf("get_tracks_feature failed\n");
        }

        puts("Top Avicii tracks:");
        for(int i=0; i < 10; i++) {
            printf("No %d: %s - ", i+1, track_vec[i].get_name().c_str());
            for(auto& aw:track_vec[i].get_artists()) printf("%s, ", aw.get_name().c_str());
            printf("\nPopularity %d. From %s: %s. Released %s\n", track_vec[i].get_popularity(), track_vec[i].get_album_appear_on().get_album_type().c_str(), track_vec[i].get_album_appear_on().get_name().c_str(), track_vec[i].get_album_appear_on().get_release_date().c_str());
            
            puts("Track feature:");
            printf("Key: %s. Duration: %llds. Tempo: %f. Loudness: %fdB\n", track_af_data[i].get_track_scale().c_str(), track_af_data[i].get_duration_ms()/1000, track_af_data[i].get_tempo(), track_af_data[i].get_loudness());
            printf("acousticness: %f, danceability: %f, energy: %f, instrumentalness: %f, liveness: %f, speechiness: %f, valence: %f\n", track_af_data[i].get_acousticness(), track_af_data[i].get_danceability(), track_af_data[i].get_energy(), track_af_data[i].get_instrumentalness(), track_af_data[i].get_liveness(), track_af_data[i].get_speechiness(), track_af_data[i].get_valence());
        }
        puts("");
    } else {
        printf("get_artist_top_tracks failed, error string: %s\n", error_string.c_str());
    }

    std::vector<artist_data> artist_vec;
    if(get_artist_related_artists(artist_id, "", artist_vec, NULL) == 1) {
        puts("Avicii related artists:");
        for(auto& av:artist_vec) {
            printf("Name: %s. %lld followers. Popularity: %d.\nGenres: ", av.get_name().c_str(), av.get_follower(), av.get_popularity());
            for(auto& avgs: av.get_genres()) {
                printf("%s, ", avgs.c_str());
            }
            puts("");
        }
    } else printf("get_artist_related_artists failed\n");

    printf("\nEND Aviici test\n");
}

void after_auth_work() {
    test_self();
    // test_avicii();
}

struct initarg {
    static bool censor_token;
    static std::string user_supplied_token;

    static int parse(int argc, char *argv[], std::string *error_string) {
        bool do_write_error_string = error_string == NULL ? 0 : 1;

        for(int i=1; i < argc; i++) {
            std::string tstr (argv[i]);
        }
    }
};

int main(int argc, char *argv[]) {

    std::string code_verifier = prepare_code_verifier(64);
    std::string generated_state;
    string authorization_url = prepare_authorization_pkce(code_verifier, generated_state);
    std::string error_string;

    int tret;
    std::string postres;
    json parsed_json;

    curl = curl_easy_init();

    if(argc >= 2) {
        token = std::string(argv[1]);
        // fprintf(stderr, "Using user-provided token %s\n", token.c_str());
        fprintf(stderr, "Using user-provided token\n");
        after_auth_work();
        return 0;
    }

    #ifdef internal_unix_based 
        tret = system( (std::string("xdg-open \"") + authorization_url + '\"').c_str() );
        if(tret != 0) {
            fprintf(stderr, "Error occured, error %d!\n", tret);
            return -1;
        }
    #elif defined(_WIN32)
        WSADATA wsaData;
        tret = WSAStartup(MAKEWORD(2,2), &wsaData);
        if(tret != 0) {
            fprintf(stderr, "WSAStartup failed: %d\n", tret);
            return -1;
        }

        CoInitialize(NULL);
        messageboxa_retry:
        if((INT_PTR) ShellExecuteA(NULL, "open", authorization_url.c_str(), NULL, NULL, SW_SHOWNORMAL) < 32) {
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
                fprintf(stderr, "[OK] Tokens obtained! Token dump: %s\n", token.c_str());
                after_auth_work();
            } else {
                fprintf(stderr, "[ERROR] Fail to request token (user authorized). Err %s\n", error_string.c_str());
            }

            curl_easy_cleanup(curl);
        } else {
            fputs("[ERROR] cURL init failed. Panic!\n", stderr);
        }
    }

    #ifdef _WIN32
    WSACleanup();
    #endif

    return 0;
}