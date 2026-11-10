#include "Library.hpp"

#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

namespace {
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) return {};
    return value;
}

int readInt(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        const auto input = readLine(prompt);
        if (!std::cin && input.empty()) return 0;
        int value = 0;
        const auto* begin = input.data();
        const auto* end = begin + input.size();
        const auto result = std::from_chars(begin, end, value);
        if (result.ec == std::errc{} && result.ptr == end && value >= minimum && value <= maximum)
            return value;
        std::cout << "Enter a whole number from " << minimum << " to " << maximum << ".\n";
    }
}

void printBooks(const std::vector<Book>& books) {
    if (books.empty()) {
        std::cout << "No books found.\n";
        return;
    }
    std::cout << "\n" << std::left << std::setw(12) << "ID" << std::setw(34) << "TITLE"
              << std::setw(26) << "AUTHOR" << "AVAILABLE / TOTAL\n";
    std::cout << std::string(88, '-') << '\n';
    for (const auto& book : books) {
        std::cout << std::left << std::setw(12) << book.id()
                  << std::setw(34) << book.title().substr(0, 32)
                  << std::setw(26) << book.author().substr(0, 24)
                  << book.availableCopies() << " / " << book.totalCopies() << '\n';
    }
}

void showMenu() {
    std::cout << "\n=== Campus Library Manager ===\n"
              << "1. View catalog\n"
              << "2. Add a book\n"
              << "3. Search catalog\n"
              << "4. Register a member\n"
              << "5. View members\n"
              << "6. Borrow or join waiting list\n"
              << "7. Return a book\n"
              << "8. View book waiting list\n"
              << "0. Save and exit\n";
}
}

int main(int argc, char* argv[]) {
    const std::string dataDirectory = argc > 1 ? argv[1] : "data";
    try {
        Library library(dataDirectory);
        std::cout << "Campus Library Manager | data folder: " << dataDirectory << '\n';
        bool running = true;
        while (running && std::cin) {
            showMenu();
            const int choice = readInt("Choose an option: ", 0, 8);
            std::string message;
            switch (choice) {
                case 1:
                    printBooks(library.allBooks());
                    break;
                case 2: {
                    const auto id = readLine("Book ID: ");
                    const auto title = readLine("Title: ");
                    const auto author = readLine("Author: ");
                    const int copies = readInt("Number of copies: ", 1, 100000);
                    library.addBook(id, title, author, copies, message);
                    std::cout << message << '\n';
                    break;
                }
                case 3: {
                    const auto query = readLine("Search by ID, title, or author: ");
                    printBooks(library.searchBooks(query));
                    break;
                }
                case 4: {
                    const auto id = readLine("Member ID: ");
                    const auto name = readLine("Member name: ");
                    library.registerMember(id, name, message);
                    std::cout << message << '\n';
                    break;
                }
                case 5: {
                    const auto members = library.allMembers();
                    if (members.empty()) std::cout << "No members registered.\n";
                    for (const auto& member : members)
                        std::cout << member.id() << " - " << member.name() << '\n';
                    break;
                }
                case 6: {
                    const auto memberId = readLine("Member ID: ");
                    const auto bookId = readLine("Book ID: ");
                    library.borrowBook(memberId, bookId, message);
                    std::cout << message << '\n';
                    break;
                }
                case 7: {
                    const auto memberId = readLine("Member ID: ");
                    const auto bookId = readLine("Book ID: ");
                    library.returnBook(memberId, bookId, message);
                    std::cout << message << '\n';
                    break;
                }
                case 8: {
                    const auto bookId = readLine("Book ID: ");
                    const auto waiting = library.waitingMembers(bookId);
                    if (waiting.empty()) std::cout << "No one is waiting for that book.\n";
                    else {
                        std::cout << "Waiting list (first in line first):\n";
                        for (std::size_t i = 0; i < waiting.size(); ++i)
                            std::cout << i + 1 << ". " << waiting[i] << '\n';
                    }
                    break;
                }
                case 0:
                    library.save();
                    std::cout << "Data saved. Goodbye.\n";
                    running = false;
                    break;
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

