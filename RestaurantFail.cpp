
#include <iostream>
#include <tuple>
#include <thread>
#include <mutex>
#include <algorithm>
#include <future>
#include <random>
#include <numeric>
#include <fstream>
#include <condition_variable>
using namespace std;
mutex mtx;
condition_variable empty, full;
// ******************* //
vector<int> globalPlate;
const int HAMBURGER = 1;
const int FRIES = 2;
const int SODA = 3;
const int meal = HAMBURGER + FRIES + SODA;
int LOOPS;
int sum = 0;
volatile bool CheffExhausted = false;
// ******************* //
tuple<int, int> getMealComponents();
// ******************* //
void producer();
vector<string> consumer(int InfiniteItem);
void put(tuple<int, int> value);
void get(vector<int> &CustomerPlate);
// ******************* //
int main(int argc, char** argv)
{
    
    if(argc != 2)
    {
        cout << "Wrong number of arguments entered!" << endl;
        exit(1);
    }
    LOOPS = stoi(argv[1]);
    
    thread Chef(producer);
    future<vector<string>> Customer1 = async(launch::async, &consumer, HAMBURGER);
    future<vector<string>> Customer2 = async(launch::async, &consumer, FRIES);
    future<vector<string>> Customer3 = async(launch::async, &consumer, SODA);
    
    if(Chef.joinable())
        Chef.join();
    vector<string> CustomerPlate1 = Customer1.get();
    vector<string> CustomerPlate2 = Customer2.get();
    vector<string> CustomerPlate3 = Customer3.get();
    
    // *** To check the state at the copmletion of the program *** //
    cout << "Total Meals consumed = " << sum << endl;
    cout << "Customer 1 is missing the following items for the meal -> ";
    for(auto itr : CustomerPlate1)
        cout << itr << ", ";
    cout << endl;
    
    cout << "Customer 2 is missing the following items for the meal -> ";
    for(auto itr : CustomerPlate2)
        cout << itr << ", ";
    cout << endl;
    
    cout << "Customer 3 is missing the following items for the meal -> ";
    for(auto itr : CustomerPlate3)
        cout << itr << ", ";
    cout << endl;
    
    return 0;
}
vector<string> consumer(int InfiniteItem)
{
    vector<int> CustomerPlate;
    CustomerPlate.push_back(InfiniteItem);
    int mealCount = 0;
    while(1)
    {
        unique_lock<mutex> lck(mtx);
        while((globalPlate.empty()) && (!CheffExhausted))
        {
            // *** To see if the deadloc happened in here ***//
            cout << "Customer " << InfiniteItem << " :: Possible Deadlock before full.wait()!" << endl;
            cout.flush();
            full.wait(lck);
            cout << "Customer " << InfiniteItem << " ::Possible Deadlock after full.wait()" << endl;
        }
        
        get(CustomerPlate);
        cout << "Notifying the Cheff (" << "globalPlate = " << globalPlate.size() << ")" << endl;
        cout.flush();
        empty.notify_all();
        if(CustomerPlate.size() == 3)
        {
            //cout << "Customer " << InfiniteItem << " finished a meal!" << endl;
            cout.flush();
            mealCount++;
            CustomerPlate.clear();
            CustomerPlate.push_back(InfiniteItem);
        }
        if(CheffExhausted)
            break;
        lck.unlock();
        
    }
    
    unique_lock<mutex> lck(mtx);
    cout << "Customer " << InfiniteItem << " is done!" << endl;
    if(CustomerPlate.size() == 3)
    {
        mealCount++;
        CustomerPlate.clear();
        CustomerPlate.push_back(InfiniteItem);
        
    }
    cout << "Meals Consumed by Customer " << InfiniteItem << " = " << mealCount << endl;
    cout.flush();
    sum += mealCount;
    lck.unlock();
    
    vector<string> MissingItems;
    if(find(CustomerPlate.begin(), CustomerPlate.end(), HAMBURGER) == CustomerPlate.end())
        MissingItems.push_back("HAMBURGER");
    if(find(CustomerPlate.begin(), CustomerPlate.end(), FRIES) == CustomerPlate.end())
        MissingItems.push_back("FRIES");
    if(find(CustomerPlate.begin(), CustomerPlate.end(), SODA) == CustomerPlate.end())
        MissingItems.push_back("SODA");
    
    return MissingItems;
}
void producer()
{
    ofstream out("testfile.txt");
    
    for(int i = 0; i < LOOPS; i++)
    {
        tuple<int, int> meal_components = getMealComponents();
        
        unique_lock<mutex> lck(mtx);
        
        while(!globalPlate.empty())
        {
            cout << "Cheff :: Possible Deadlock before empty.wait! (" << "globalPlate Size = " << globalPlate.size() << ")" << endl;
            cout.flush();
            empty.wait(lck);
            cout << "Cheff :: Possible Deadlock after empty.wait! (" << "globalPlate Size = " << globalPlate.size() << ")" << endl;
            cout.flush();
        }
        put(meal_components);
        //*** To keep track of what sequence of items are causing a deadlock ***//
        for(auto itr : globalPlate)
            out << itr << "\t";
        out << endl;
        out.flush();
        full.notify_all();
        
        lck.unlock();
    }
    
    unique_lock<mutex> lck(mtx);
    CheffExhausted = true;
    cout << "CHEF EXHAUSTED !" << endl;
    full.notify_all();                      // Bcs some Customers might be waiting on the food and need to be notified
    lck.unlock();
    out.close();
}
void get(vector<int> &CustomerPlate)
{
    for (auto itr : globalPlate)
    {
        if (find(CustomerPlate.begin(), CustomerPlate.end(), itr) == CustomerPlate.end())
        {
            CustomerPlate.push_back(itr);
            globalPlate.erase(find(globalPlate.begin(), globalPlate.end(), itr));
            break;
        }
    }
    cout << "Irritating Item = " << globalPlate[0] << endl;
    cout << "Customer " << CustomerPlate[0] << " plate -> ";
    for(auto itr : CustomerPlate)
        cout << itr << ", ";
    cout << endl;
    cout.flush();
}
void put(tuple<int, int> value)
{
    globalPlate.push_back(get<0>(value));
    globalPlate.push_back(get<1>(value));
}
tuple<int, int> getMealComponents()
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



