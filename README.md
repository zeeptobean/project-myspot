Implementing Spotify API in C++. Desktop app working in progress


### Dependencies
- [nlohmann's JSON](https://github.com/nlohmann/json)
- [cURL](https://github.com/curl/curl)
- OpenSSL

### Build
Compile `spotcheck.cpp` file for API test

### Technical details
- Authorization through PCKE flow. Refreshing token not supported yet
- Client credentials authorization is temporary unavailable due to conflicting feature
- The API test code would prompt the user for authorization, then open a simple HTTP server for callback

Define `APP_CLIENT_ID` for your own app id

By default web server for callback would be `http://127.0.0.1:62309/` (set this in the **Redirect URIs** section of your own Spotify app)

Click [here](https://developer.spotify.com/documentation/web-api/concepts/apps) for more information