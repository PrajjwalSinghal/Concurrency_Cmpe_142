
#include <iostream>
#include <tuple>
#include <thread>
#include <mutex>
#include <algorithm>
#include <future>
#include <random>
#include <condition_variable>
using namespace std;
mutex mtx;                              // mutex to implement locks
condition_variable empty, full;         // condition variable to wake the producer and consumer threads
// ******************* //
vector<int> globalPlate;                // Chef puts the Meal Components on Global Plate
const int HAMBURGER = 1;
const int FRIES = 2;
const int SODA = 3;
const int meal = HAMBURGER + FRIES + SODA;
int missingItem = 0;
int LOOPS;                              // The number of items the chef will produce
int sum = 0;                            // Sum of the number of meals consumed by the customer
volatile bool CheffExhausted = false;   // To check if Chef will no longer produce any item or not
// ******************* //
tuple<int, int> getMealComponents();    // Returns 2 random meal components
// ******************* //
void producer();                        // Function to implement Chef
void consumer(int InfiniteItem);        // Function to implemet Customers
void put(tuple<int, int> value);        // Funtion to put items on global Plate
void get(vector<int> &CustomerPlate);   // Funtion to get items from global Plate and put them on Customer Plate
// ******************* //
int main(int argc, char** argv)
{
    
    if(argc != 2)
    {
        cout << "Wrong number of arguments entered!" << endl;
        exit(1);
    }
    LOOPS = stoi(argv[1]);
    
    // *** Start the 4 threads ***//
    thread Chef(producer);
    thread Customer1(consumer, HAMBURGER);
    thread Customer2(consumer, FRIES);
    thread Customer3(consumer, SODA);
    
    // *** Joining the 4 threads ***//
    if(Chef.joinable())
        Chef.join();
    
    if(Customer1.joinable())
        Customer1.join();
    
    if(Customer2.joinable())
        Customer2.join();
    
    if(Customer3.joinable())
        Customer3.join();
    
    cout << "Total Meals consumed = " << sum << endl;
    return 0;
}
void consumer(int InfiniteItem)
{
    vector<int> CustomerPlate;
    CustomerPlate.push_back(InfiniteItem);
    int mealCount = 0;
    while(1)
    {
        // ******** lock ********** //
        unique_lock<mutex> lck(mtx);
        
        while((globalPlate.empty()) && (missingItem != InfiniteItem) && (!CheffExhausted))
            full.wait(lck);
        if(!globalPlate.empty())
            get(CustomerPlate);
        empty.notify_all();
        if(CustomerPlate.size() == 3)
        {
            mealCount++;
            CustomerPlate.clear();
            CustomerPlate.push_back(InfiniteItem);
            missingItem = 0;
        }
        // ******** unlock ********** //
        lck.unlock();
        
        if(CheffExhausted)
            break;
    }
    
    // ******** lock ********** //
    unique_lock<mutex> lck(mtx);
    if(CustomerPlate.size() == 3)
    {
        mealCount++;
        CustomerPlate.clear();
        CustomerPlate.push_back(InfiniteItem);
    }
    cout << "Meals Consumed by Customer " << InfiniteItem << " = " << mealCount << endl;
    cout.flush();
    sum += mealCount;
    // ******** unlock ********** //
    lck.unlock();
}
void producer()
{
    for(int i = 0; i < LOOPS; i++)
    {
        tuple<int, int> meal_components = getMealComponents();
        
        // ******** lock ********** //
        unique_lock<mutex> lck(mtx);
        while(!globalPlate.empty())
            empty.wait(lck);
        put(meal_components);
        missingItem = meal - accumulate(globalPlate.begin(), globalPlate.end(), 0);
        full.notify_all();
        // ******** unlock ********** //
        lck.unlock();
    }
    // ******** lock ********** //
    unique_lock<mutex> lck(mtx);
    CheffExhausted = true;
    cout << "CHEF EXHAUSTED !" << endl;
    full.notify_all();                      // Bcs some Customers might be waiting on the food and need to be notified
    // ******** unlock ********** //
    lck.unlock();
}
void get(vector<int> &CustomerPlate)
{
    CustomerPlate.push_back(globalPlate[0]);
    CustomerPlate.push_back(globalPlate[1]);
    globalPlate.clear();
}
void put(tuple<int, int> value)
{
    globalPlate.push_back(get<0>(value));
    globalPlate.push_back(get<1>(value));
}
tuple<int, int> getMealComponents()             // Randomly generate two meal components
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(HAMBURGER,SODA);
    
    int num1 = dis(gen);
    int num2 = dis(gen);
    while(num2 == num1)
        num2 = dis(gen);
    tuple<int, int> MealComponents(num1, num2);
    return MealComponents;
}
