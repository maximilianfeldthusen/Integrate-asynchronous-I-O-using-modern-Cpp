## Documentation 

### Integrate-asynchronous-I-O-using-modern-Cpp

### 1. Header Inclusions and Type Alias

```cpp
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <future>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
```

- **`<iostream>`:** Provides input/output capabilities, allowing the code to print messages to the console.
- **`<stdexcept>`:** Offers standard exception classes (like `std::runtime_error`), which are used here for error handling.
- **`<cstdlib>`:** Contains utility functions such as `std::getenv` to access environment variables.
- **`<string>`:** Enables the use of C++'s `std::string` for text manipulation.
- **`<future>`:** Supports asynchronous operations using `std::async` and `std::future`, letting the code perform a network call without blocking the main thread.
- **`<curl/curl.h>`:** Is the header for libcurl, which handles HTTP requests.
- **`<nlohmann/json.hpp>`:** Is a popular header-only JSON library that makes it easy to build, parse, and manipulate JSON objects in C++.

The statement:
```cpp
using json = nlohmann::json;
```
defines an alias so that we can refer to `nlohmann::json` simply as `json` throughout the code.

---

### 2. The WriteCallback Function

```cpp
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* mem = static_cast<std::string*>(userp);
    mem->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
```

- **Purpose:**  
  This callback is used by libcurl to process chunks of data received from an HTTP request.

- **How It Works:**
  - **Parameters:**
    - `contents`: Points to the raw data received.
    - `size` **and** `nmemb`: Together they determine the total size of the incoming data (`totalSize = size * nmemb`).
    - `userp`: A pointer that we expect to be a `std::string` where the data will be appended.
  - **Operation:**  
    The function casts `userp` to a `std::string*` and appends the new data to it. Finally, it returns the number of bytes processed which tells libcurl that the data was handled correctly.

---

### 3. The `callOpenAICompletionAPI` Function

This function builds and sends an asynchronous HTTP POST request to the OpenAI API and returns the response as a string.

```cpp
std::string callOpenAICompletionAPI(const std::string& prompt) {
    // Retrieve the API key securely from an environment variable.
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        throw std::runtime_error("Environment variable OPENAI_API_KEY not set.");
    }
```

- **API Key Retrieval:**  
  It uses `std::getenv` to retrieve the API key from the environment variable `OPENAI_API_KEY`. If the key isn’t found, it throws a runtime error. This keeps sensitive credentials out of the source code.

```cpp
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL.");
    }

    std::string readBuffer;
    CURLcode res;
    struct curl_slist* headers = nullptr;
```

- **Initialization:**
  - A CURL handle is initialized with `curl_easy_init()`. Failure to initialize results in an exception.
  - `readBuffer` is declared to store the response.
  - `headers` will hold custom HTTP headers.

#### Inside the Try Block

```cpp
    try {
        const std::string url = "https://api.openai.com/v1/completions";

        json requestData = {
            {"model", "text-davinci-003"},
            {"prompt", prompt},
            {"max_tokens", 150},
            {"temperature", 0.7}
        };

        std::string requestBody = requestData.dump();
```

- **Request Preparation:**  
  - **URL:** The API endpoint is defined as a constant string.
  - **JSON Request:**  
    A JSON object, `requestData`, is created containing parameters such as the model name, prompt, maximum allowed tokens for the response, and temperature. The JSON object is then converted (serialized) to a string called `requestBody`.

```cpp
        // Set CURL options.
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
```

- **Setting Request Options:**  
  These options instruct libcurl:
  - To use the specified URL.
  - To use the POST method.
  - To set the body of the POST request to be the JSON string.

```cpp
        // Build and set HTTP headers.
        std::string authHeader = "Authorization: Bearer " + std::string(api_key);
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
```

- **HTTP Headers:**
  - An Authorization header is built including the bearer token.
  - A Content-Type header is added to specify the JSON format.
  - These headers are then attached to the request.

```cpp
        // Register callback for writing received data.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
```

- **Registering the Callback:**  
  The callback (`WriteCallback`) is set so that the received data is written into `readBuffer`.

```cpp
        // Enforce secure TLS certificate verification.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
```

- **TLS Security:**  
  These options enforce verification of the SSL/TLS certificate, ensuring a secure connection.

```cpp
        // Execute the HTTP POST request.
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("CURL error: " + std::string(curl_easy_strerror(res)));
        }
```

- **Executing the Request:**  
  The API call is performed with `curl_easy_perform(curl)`. If any error occurs, an exception is thrown containing the relevant error message.

```cpp
        // Cleanup HTTP headers.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
```

- **Cleanup:**  
  After the transaction, the allocated headers and CURL session are freed, which helps prevent memory leaks.

#### Exception Handling

```cpp
    catch (...) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);
        throw;
    }
```

- **Resource Management on Error:**  
  If an exception is thrown, the catch block ensures that any allocated resources (like headers or the CURL handle) are properly cleaned up before rethrowing the exception.

- **Return Value:**  
  Finally, `readBuffer`, which now holds the response from the API, is returned.

---

### 4. The `main` Function

```cpp
int main() {
    try {
        std::string prompt = "Explain best practices for writing secure C++ code with a focus on asynchronous I/O, including memory management, error handling, and secure API design.";
        std::cout << "Sending asynchronous request to OpenAI API...\n" << std::endl;
```

- **Defining the Prompt:**  
  A string `prompt` is defined which contains the question sent to the API.
- **Output:**  
  A message is printed to indicate that the asynchronous request is being sent.

```cpp
        // Launch the API call asynchronously.
        std::future<std::string> futureResponse = std::async(std::launch::async, callOpenAICompletionAPI, prompt);
```

- **Asynchronous Operation:**  
  The API call is launched asynchronously using `std::async`. This creates a separate thread that will execute `callOpenAICompletionAPI` and returns a `std::future` that will eventually hold the result.

```cpp
        // Wait and get the result.
        std::string response = futureResponse.get();
```

- **Waiting for the Response:**  
  Using `futureResponse.get()` blocks until the asynchronous operation completes, then retrieves the response.

```cpp
        // Parse and pretty-print the JSON response.
        json jsonResponse = json::parse(response);
        std::cout << "Asynchronous OpenAI API Response:\n" << jsonResponse.dump(4) << std::endl;
```

- **JSON Parsing:**  
  The response (assumed to be a JSON-formatted string) is parsed into a JSON object.
- **Pretty Printing:**  
  The JSON is printed to the console with an indentation of 4 spaces for readability.

```cpp
    }
    catch (const std::exception& ex) {
        std::cerr << "An error occurred: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

- **Error Handling in `main`:**  
  Any exceptions thrown during the process are caught, an error message is printed, and the program returns a failure code.
- **Successful Exit:**  
  If everything runs correctly, the program returns `EXIT_SUCCESS` (typically zero).

---

### Summary

- **Security Practices:**  
  - Retrieves API keys from environment variables.
  - Enforces TLS verification when connecting to the API.

- **Asynchronous Execution:**  
  - Uses `std::async` and `std::future` to run the network call in a separate thread, keeping the application responsive.

- **Libcurl and JSON:**  
  - Utilizes libcurl to perform HTTP requests and employs a callback to accumulate the response.
  - Uses nlohmann/json to build and parse JSON data effortlessly.

- **Resource Management and Error Handling:**  
  - Proper management of CURL resources (using cleanup calls) ensures no memory leaks.
  - Try-catch blocks gracefully handle errors by releasing resources and providing informative error messages.
