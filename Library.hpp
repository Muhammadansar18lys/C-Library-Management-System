#pragma once

#include <filesystem>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Book {
public:
    Book(std::string id, std::string title, std::string author, int copies);

    const std::string& id() const noexcept;
    const std::string& title() const noexcept;
    const std::string& author() const noexcept;
    int totalCopies() const noexcept;
    int availableCopies() const noexcept;
    bool borrowCopy() noexcept;
    void returnCopy() noexcept;

private:
    std::string id_;
    std::string title_;
    std::string author_;
    int totalCopies_;
    int availableCopies_;
};

class Member {
public:
    Member(std::string id, std::string name);

    const std::string& id() const noexcept;
    const std::string& name() const noexcept;

private:
    std::string id_;
    std::string name_;
};

class Library {
public:
    explicit Library(std::filesystem::path dataDirectory = "data");

    bool addBook(const std::string& id, const std::string& title,
                 const std::string& author, int copies, std::string& message);
    bool registerMember(const std::string& id, const std::string& name,
                        std::string& message);
    std::vector<Book> allBooks() const;
    std::vector<Book> searchBooks(const std::string& query) const;
    std::vector<Member> allMembers() const;
    bool borrowBook(const std::string& memberId, const std::string& bookId,
                    std::string& message);
    bool returnBook(const std::string& memberId, const std::string& bookId,
                    std::string& message);
    std::vector<std::string> waitingMembers(const std::string& bookId) const;
    void save() const;

private:
    std::filesystem::path dataDirectory_;
    std::vector<Book> books_;
    std::unordered_map<std::string, std::size_t> bookPosition_;
    std::unordered_map<std::string, Member> members_;
    std::unordered_map<std::string, std::unordered_set<std::string>> loansByBook_;
    std::unordered_map<std::string, std::unordered_set<std::string>> loansByMember_;
    std::unordered_map<std::string, std::queue<std::string>> waitingLists_;

    void load();
    Book* findBook(const std::string& id);
    const Book* findBook(const std::string& id) const;
    static std::string lower(std::string value);
};
