#include "Library.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <utility>

Book::Book(std::string id, std::string title, std::string author, int copies)
    : id_(std::move(id)), title_(std::move(title)), author_(std::move(author)),
      totalCopies_(copies), availableCopies_(copies) {
    if (copies < 1) {
        throw std::invalid_argument("A book must have at least one copy.");
    }
}

const std::string& Book::id() const noexcept { return id_; }
const std::string& Book::title() const noexcept { return title_; }
const std::string& Book::author() const noexcept { return author_; }
int Book::totalCopies() const noexcept { return totalCopies_; }
int Book::availableCopies() const noexcept { return availableCopies_; }

bool Book::borrowCopy() noexcept {
    if (availableCopies_ == 0) return false;
    --availableCopies_;
    return true;
}

void Book::returnCopy() noexcept {
    if (availableCopies_ < totalCopies_) ++availableCopies_;
}

Member::Member(std::string id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {}
const std::string& Member::id() const noexcept { return id_; }
const std::string& Member::name() const noexcept { return name_; }

Library::Library(std::filesystem::path dataDirectory)
    : dataDirectory_(std::move(dataDirectory)) {
    load();
}

bool Library::addBook(const std::string& id, const std::string& title,
                      const std::string& author, int copies, std::string& message) {
    if (id.empty() || title.empty() || author.empty() || copies < 1) {
        message = "Enter an ID, title, author, and a copy count of at least 1.";
        return false;
    }
    if (bookPosition_.contains(id)) {
        message = "That book ID already exists.";
        return false;
    }
    bookPosition_[id] = books_.size();
    books_.emplace_back(id, title, author, copies);
    message = "Book added to the catalog.";
    save();
    return true;
}

bool Library::registerMember(const std::string& id, const std::string& name,
                             std::string& message) {
    if (id.empty() || name.empty()) {
        message = "Enter both a member ID and name.";
        return false;
    }
    if (members_.contains(id)) {
        message = "That member ID is already registered.";
        return false;
    }
    members_.emplace(id, Member{id, name});
    message = "Member registered.";
    save();
    return true;
}

std::vector<Book> Library::allBooks() const {
    auto result = books_;
    std::sort(result.begin(), result.end(), [](const Book& a, const Book& b) {
        if (lower(a.title()) != lower(b.title())) return lower(a.title()) < lower(b.title());
        return a.id() < b.id();
    });
    return result;
}

std::vector<Book> Library::searchBooks(const std::string& query) const {
    const auto needle = lower(query);
    std::vector<Book> result;
    for (const auto& book : books_) {
        if (lower(book.id()).find(needle) != std::string::npos ||
            lower(book.title()).find(needle) != std::string::npos ||
            lower(book.author()).find(needle) != std::string::npos) {
            result.push_back(book);
        }
    }
    std::sort(result.begin(), result.end(), [](const Book& a, const Book& b) {
        return lower(a.title()) < lower(b.title());
    });
    return result;
}

std::vector<Member> Library::allMembers() const {
    std::vector<Member> result;
    result.reserve(members_.size());
    for (const auto& [id, member] : members_) {
        (void)id;
        result.push_back(member);
    }
    std::sort(result.begin(), result.end(), [](const Member& a, const Member& b) {
        if (lower(a.name()) != lower(b.name())) return lower(a.name()) < lower(b.name());
        return a.id() < b.id();
    });
    return result;
}

bool Library::borrowBook(const std::string& memberId, const std::string& bookId,
                         std::string& message) {
    auto member = members_.find(memberId);
    auto* book = findBook(bookId);
    if (member == members_.end()) {
        message = "Member ID not found. Register the member first.";
        return false;
    }
    if (book == nullptr) {
        message = "Book ID not found.";
        return false;
    }
    if (loansByMember_[memberId].contains(bookId)) {
        message = "This member already has a copy of that book.";
        return false;
    }

    auto& queue = waitingLists_[bookId];
    if (!queue.empty() || book->availableCopies() == 0) {
        auto scan = queue;
        while (!scan.empty()) {
            if (scan.front() == memberId) {
                message = "This member is already on the waiting list.";
                return false;
            }
            scan.pop();
        }
        queue.push(memberId);
        message = "No copy is available. Member added to the waiting list.";
        save();
        return true;
    }

    book->borrowCopy();
    loansByBook_[bookId].insert(memberId);
    loansByMember_[memberId].insert(bookId);
    message = "Book checked out successfully.";
    save();
    return true;
}

bool Library::returnBook(const std::string& memberId, const std::string& bookId,
                         std::string& message) {
    auto member = members_.find(memberId);
    auto* book = findBook(bookId);
    if (member == members_.end()) {
        message = "Member ID not found.";
        return false;
    }
    if (book == nullptr) {
        message = "Book ID not found.";
        return false;
    }
    auto active = loansByBook_.find(bookId);
    if (active == loansByBook_.end() || !active->second.contains(memberId)) {
        message = "This member does not have that book checked out.";
        return false;
    }

    active->second.erase(memberId);
    loansByMember_[memberId].erase(bookId);
    book->returnCopy();

    auto& queue = waitingLists_[bookId];
    while (!queue.empty()) {
        const auto nextId = queue.front();
        queue.pop();
        auto nextMember = members_.find(nextId);
        if (nextMember == members_.end() || loansByMember_[nextId].contains(bookId)) continue;
        if (book->borrowCopy()) {
            loansByBook_[bookId].insert(nextId);
            loansByMember_[nextId].insert(bookId);
            message = "Book returned. It was automatically checked out to the next member on the waiting list (" + nextId + ").";
            save();
            return true;
        }
    }

    message = "Book returned successfully.";
    save();
    return true;
}

std::vector<std::string> Library::waitingMembers(const std::string& bookId) const {
    std::vector<std::string> result;
    auto found = waitingLists_.find(bookId);
    if (found == waitingLists_.end()) return result;
    auto queue = found->second;
    while (!queue.empty()) {
        result.push_back(queue.front());
        queue.pop();
    }
    return result;
}

void Library::save() const {
    std::filesystem::create_directories(dataDirectory_);
    {
        std::ofstream out(dataDirectory_ / "books.db", std::ios::trunc);
        if (!out) throw std::runtime_error("Unable to write books.db");
        for (const auto& book : books_)
            out << std::quoted(book.id()) << ' ' << std::quoted(book.title()) << ' '
                << std::quoted(book.author()) << ' ' << book.totalCopies() << '\n';
    }
    {
        std::ofstream out(dataDirectory_ / "members.db", std::ios::trunc);
        if (!out) throw std::runtime_error("Unable to write members.db");
        for (const auto& [id, member] : members_)
            out << std::quoted(id) << ' ' << std::quoted(member.name()) << '\n';
    }
    {
        std::ofstream out(dataDirectory_ / "loans.db", std::ios::trunc);
        if (!out) throw std::runtime_error("Unable to write loans.db");
        for (const auto& [bookId, memberIds] : loansByBook_)
            for (const auto& memberId : memberIds)
                out << std::quoted(bookId) << ' ' << std::quoted(memberId) << '\n';
    }
    {
        std::ofstream out(dataDirectory_ / "waiting-list.db", std::ios::trunc);
        if (!out) throw std::runtime_error("Unable to write waiting-list.db");
        for (const auto& [bookId, waiting] : waitingLists_) {
            auto queue = waiting;
            while (!queue.empty()) {
                out << std::quoted(bookId) << ' ' << std::quoted(queue.front()) << '\n';
                queue.pop();
            }
        }
    }
}

void Library::load() {
    if (!std::filesystem::exists(dataDirectory_)) return;
    {
        std::ifstream in(dataDirectory_ / "books.db");
        std::string id, title, author;
        int copies = 0;
        while (in >> std::quoted(id) >> std::quoted(title) >> std::quoted(author) >> copies) {
            if (id.empty() || copies < 1 || bookPosition_.contains(id)) continue;
            bookPosition_[id] = books_.size();
            books_.emplace_back(id, title, author, copies);
        }
    }
    {
        std::ifstream in(dataDirectory_ / "members.db");
        std::string id, name;
        while (in >> std::quoted(id) >> std::quoted(name)) {
            if (!id.empty() && !name.empty()) members_.try_emplace(id, id, name);
        }
    }
    {
        std::ifstream in(dataDirectory_ / "loans.db");
        std::string bookId, memberId;
        while (in >> std::quoted(bookId) >> std::quoted(memberId)) {
            auto* book = findBook(bookId);
            if (book == nullptr || !members_.contains(memberId) || book->availableCopies() == 0) continue;
            if (loansByBook_[bookId].insert(memberId).second) {
                loansByMember_[memberId].insert(bookId);
                book->borrowCopy();
            }
        }
    }
    {
        std::ifstream in(dataDirectory_ / "waiting-list.db");
        std::string bookId, memberId;
        while (in >> std::quoted(bookId) >> std::quoted(memberId)) {
            if (findBook(bookId) == nullptr || !members_.contains(memberId)) continue;
            waitingLists_[bookId].push(memberId);
        }
    }
}

Book* Library::findBook(const std::string& id) {
    auto found = bookPosition_.find(id);
    return found == bookPosition_.end() ? nullptr : &books_[found->second];
}

const Book* Library::findBook(const std::string& id) const {
    auto found = bookPosition_.find(id);
    return found == bookPosition_.end() ? nullptr : &books_[found->second];
}

std::string Library::lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

