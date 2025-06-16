
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <future>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Callback function to collect the response data from libcurl.
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* mem = static_cast<std::string*>(userp);
    mem->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Function to perform the API call to OpenAI asynchronously.
std::string callOpenAICompletionAPI(const std::string& prompt) {
    // Retrieve the API key securely from an environment variable.
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        throw std::runtime_error("Environment variable OPENAI_API_KEY not set.");
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL.");
    }

    std::string readBuffer;
    CURLcode res;
    struct curl_slist* headers = nullptr;

    try {
        const std::string url = "https://api.openai.com/v1/completions";

        json requestData = {
            {"model", "text-davinci-003"},
            {"prompt", prompt},
            {"max_tokens", 150},
            {"temperature", 0.7}
        };

        std::string requestBody = requestData.dump();

        // Set CURL options.
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());

        // Build and set HTTP headers.
        std::string authHeader = "Authorization: Bearer " + std::string(api_key);
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Register callback for writing received data.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Enforce secure TLS certificate verification.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Execute the HTTP POST request.
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("CURL error: " + std::string(curl_easy_strerror(res)));
        }

        // Cleanup HTTP headers.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    catch (...) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(curl);
        throw;
    }

    return readBuffer;
}

int main() {
    try {
        // Define the prompt with a focus on secure coding, including asynchronous I/O.
        std::string prompt = "Explain best practices for writing secure C++ code with a focus on asynchronous I/O, including memory management, error handling, and secure API design.";
        std::cout << "Sending asynchronous request to OpenAI API...\n" << std::endl;

        // Launch the API call asynchronously.
        std::future<std::string> futureResponse = std::async(std::launch::async, callOpenAICompletionAPI, prompt);

        // In a more complex application, other tasks can be performed here while waiting.
        // For demonstration, we simply wait for the result.
        std::string response = futureResponse.get();

        // Parse and pretty-print the JSON response.
        json jsonResponse = json::parse(response);
        std::cout << "Asynchronous OpenAI API Response:\n" << jsonResponse.dump(4) << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "An error occurred: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
