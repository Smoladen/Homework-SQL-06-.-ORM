#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <iostream>
#include <memory>
#include <ctime>

namespace dbo = Wt::Dbo;

class Publisher;
class Book;
class Shop;
class Stock;
class Sale;

class Publisher {
public:
    std::string name;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, name, "name");
        dbo::hasMany(a, books, dbo::ManyToOne, "id_publisher");
    }

    dbo::collection<dbo::ptr<Book>> books;
};

class Book {
public:
    std::string title;

    dbo::ptr<Publisher> publisher;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, title, "title");
        dbo::belongsTo(a, publisher, "id_publisher");
        dbo::hasMany(a, stocks, dbo::ManyToOne, "id_book");
    }

    dbo::collection<dbo::ptr<Stock>> stocks;
};

class Shop {
public:
    std::string name;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, name, "name");
        dbo::hasMany(a, stocks, dbo::ManyToOne, "id_shop");
    }

    dbo::collection<dbo::ptr<Stock>> stocks;
};

class Stock {
public:
    int count;

    dbo::ptr<Book> book;
    dbo::ptr<Shop> shop;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, count, "count");
        dbo::belongsTo(a, book, "id_book");
        dbo::belongsTo(a, shop, "id_shop");
        dbo::hasMany(a, sales, dbo::ManyToOne, "id_stock");
    }

    dbo::collection<dbo::ptr<Sale>> sales;
};

class Sale {
public:
    double price;
    std::time_t dateSale;  
    int count;

    dbo::ptr<Stock> stock;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, price, "price");
        dbo::field(a, dateSale, "date_sale");
        dbo::field(a, count, "count");
        dbo::belongsTo(a, stock, "id_stock");
    }
};



int main() {

    std::string connectionString = "host=localhost port=5432 dbname=ORMTest user=postgres password=Smolkin@2";

    auto postgres = std::make_unique<dbo::backend::Postgres>(connectionString);
    dbo::Session session;
    session.setConnection(std::move(postgres));

    session.mapClass<Publisher>("publisher");
    session.mapClass<Book>("book");
    session.mapClass<Shop>("shop");
    session.mapClass<Stock>("stock");
    session.mapClass<Sale>("sale");

    session.createTables();

    { 
        dbo::Transaction transaction{ session };

        std::unique_ptr<Publisher> publisher(new Publisher());
        publisher->name = "Example Publisher";
        dbo::ptr<Publisher> publisherPtr = session.add(std::move(publisher));

        std::unique_ptr<Book> book(new Book());
        book->title = "Example Book";
        book->publisher = publisherPtr;
        dbo::ptr<Book> bookPtr = session.add(std::move(book));

        std::unique_ptr<Shop> shop(new Shop());
        shop->name = "Example Shop";
        dbo::ptr<Shop> shopPtr = session.add(std::move(shop));

        std::unique_ptr<Stock> stock(new Stock());
        stock->count = 100;
        stock->book = bookPtr;
        stock->shop = shopPtr;
        dbo::ptr<Stock> stockPtr = session.add(std::move(stock));

        std::unique_ptr<Sale> sale(new Sale());
        sale->price = 9.99;
        sale->dateSale = std::time(nullptr);
        sale->count = 10;
        sale->stock = stockPtr;
        session.add(std::move(sale));

        transaction.commit();
    } 

    std::string publisherName;
    std::cout << "Enter publisher name: ";
    std::cin >> publisherName;
    { 
        dbo::Transaction transaction{ session };

        dbo::ptr<Publisher> targetPublisher = session.find<Publisher>().where("name = ?").bind(publisherName);
        if (targetPublisher) {
            std::cout << "Publisher: " << targetPublisher->name << std::endl;

            for (const dbo::ptr<Book>& book : targetPublisher->books) {
                for (const dbo::ptr<Stock>& stock : book->stocks) {
                    std::cout << "Shop: " << stock->shop->name << " sells the book: " << book->title << std::endl;
                }
            }
        }
        else {
            std::cout << "Publisher not found." << std::endl;
        }

        transaction.commit();
    }
    return 0;
}