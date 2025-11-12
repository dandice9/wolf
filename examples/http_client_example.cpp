#include "../src/http_client.hpp"
#include <iostream>

int main() {
    try {
        // GET request example
        std::cout << "=== GET Request ===" << std::endl;
        auto get_response = wolf::client::get_request("http://httpbin.org/get");
        std::cout << "Status: " << get_response.result_int() << std::endl;
        std::cout << "Body: " << get_response.body() << std::endl << std::endl;

        // POST request example
        std::cout << "=== POST Request ===" << std::endl;
        json::object post_data = {
            {"name", "John Doe"},
            {"email", "john@example.com"},
            {"message", "Hello from Wolf HTTP Client!"}
        };
        std::string post_body = json::serialize(post_data);
        
        auto post_response = wolf::client::post_request("http://httpbin.org/post", post_body);
        std::cout << "Status: " << post_response.result_int() << std::endl;
        std::cout << "Body: " << post_response.body() << std::endl << std::endl;

        // PUT request example
        std::cout << "=== PUT Request ===" << std::endl;
        json::object put_data = {
            {"id", 1},
            {"name", "Updated Name"},
            {"status", "active"}
        };
        std::string put_body = json::serialize(put_data);
        
        auto put_response = wolf::client::put_request("http://httpbin.org/put", put_body);
        std::cout << "Status: " << put_response.result_int() << std::endl;
        std::cout << "Body: " << put_response.body() << std::endl << std::endl;

        // PATCH request example
        std::cout << "=== PATCH Request ===" << std::endl;
        json::object patch_data = {
            {"status", "inactive"}
        };
        std::string patch_body = json::serialize(patch_data);
        
        auto patch_response = wolf::client::patch_request("http://httpbin.org/patch", patch_body);
        std::cout << "Status: " << patch_response.result_int() << std::endl;
        std::cout << "Body: " << patch_response.body() << std::endl << std::endl;

        // DELETE request example
        std::cout << "=== DELETE Request ===" << std::endl;
        auto delete_response = wolf::client::delete_request("http://httpbin.org/delete");
        std::cout << "Status: " << delete_response.result_int() << std::endl;
        std::cout << "Body: " << delete_response.body() << std::endl << std::endl;

        // Example with custom content type
        std::cout << "=== POST with form-data ===" << std::endl;
        std::string form_data = "name=John&email=john@example.com";
        auto form_response = wolf::client::post_request(
            "http://httpbin.org/post", 
            form_data, 
            "application/x-www-form-urlencoded"
        );
        std::cout << "Status: " << form_response.result_int() << std::endl;
        std::cout << "Body: " << form_response.body() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
