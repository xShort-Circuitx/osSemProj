#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <condition_variable>
#include <semaphore>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <queue>


// Constants
const int MAX_QUEUE_SIZE = 50;
const int MAX_MEMORY_SIZE = 100;
const int MAX_CARTS = 5;

// Global synchronization mutex
std::mutex globalMutex;
std::mutex CartsMutex;
std::mutex WorkersMutex;
std::mutex weatherMutex;
std::condition_variable weatherCV;



// Global resources
std::atomic<int> globalBricks(100);
std::atomic<int> globalCement(50);
std::atomic<int> globalTools(20);
std::atomic<bool> Carts[MAX_CARTS];
std::atomic<bool> Workers[10];

// Global variables
bool adverseWeatherConditions = false;
int sharedMemory[MAX_MEMORY_SIZE];
std::condition_variable check_cart;
std::condition_variable check_resource;


class ConstructionTask;
class Worker;
class Building;
class ResourceManager;
class PriorityScheduler;
class IOManager;
class DynamicResourceGenerator;
class TaskAssigner;


class ResourceManager {
public:
    bool addResources(int bricksToAdd, int cementToAdd, int toolsToAdd);
    void useResources(int bricksToUse, int cementToUse, int toolsToUse);
};

class PriorityScheduler {
public:
    // Create three queues as per the priority levels
    ConstructionTask* lowPriorityQueue[MAX_QUEUE_SIZE];
    ConstructionTask* mediumPriorityQueue[MAX_QUEUE_SIZE];
    ConstructionTask* highPriorityQueue[MAX_QUEUE_SIZE];

    // Create three counters to keep track of the number of tasks in each queue
    int lowPriorityCount;
    int mediumPriorityCount;
    int highPriorityCount;

    PriorityScheduler() : lowPriorityCount(0), mediumPriorityCount(0), highPriorityCount(0) {}

    void addTask(ConstructionTask* task);
    ConstructionTask* getNextTask();
};



class IOManager {
public:
    void simulateWeatherConditions();
    bool isAdverseWeather();
};


class DynamicResourceGenerator {
public:
    void generateResources();
};

class Worker {
public:
    std::string workerName;
    int skillLevel;
    bool onBreak;

    Worker(std::string name, int skill) : workerName(name), skillLevel(skill), onBreak(false) {}

    bool PerformTask(ConstructionTask* task);
    
};

class ConstructionTask {
public:
    std::string taskName;
    int priorityLevel;
    int requiredBricks;
    int requiredCement;
    int requiredTools;

    ConstructionTask(std::string name, int priority, int bricks, int cement, int tools)
        : taskName(name), priorityLevel(priority), requiredBricks(bricks), requiredCement(cement), requiredTools(tools) {}

    //void operator()();
};

class TaskAssigner {
public:
    void assignTask(ConstructionTask* task, Worker* workers, int numWorkers);
};

class Building {
public:
    std::string buildingName;
    ConstructionTask* tasks;
    int tasksRequired;
    int tasksCompleted;

    // Default constructor
    Building() : buildingName("DefaultBuilding"), tasksRequired(0), tasksCompleted(0) {}

    // Parameterized constructor
    Building(std::string name, int tasks)
        : buildingName(name), tasksRequired(tasks), tasksCompleted(0) {}

    void constructBuilding();
    bool isConstructionComplete();
};

void IOManager::simulateWeatherConditions() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> weatherDistribution(1, 10);

    int weatherValue = weatherDistribution(gen);

    std::unique_lock<std::mutex> lock(weatherMutex);
    if (weatherValue <= 3) {
        adverseWeatherConditions = true;
        std::cout << "Adverse weather conditions detected!\n";
        weatherCV.notify_all(); // Notify all waiting threads
    } else {
        adverseWeatherConditions = false;
        std::cout << "Weather conditions are favorable.\n";
    }
}

bool IOManager::isAdverseWeather() {
    std::unique_lock<std::mutex> lock(weatherMutex);
    return adverseWeatherConditions;
}


bool ResourceManager::addResources(int bricksToAdd, int cementToAdd, int toolsToAdd) {

    bool cartAvailable = false;
    int cartNumber = 0;

    
    for(int i=0;i<MAX_CARTS;i++)
    {
        if(Carts[i]==false)
        {
            cartAvailable = true;
            cartNumber = i;
            Carts[i] = true;
            break;
        }
    }

    if(cartAvailable){
        // Add the specified quantities of each resource
        globalBricks += bricksToAdd;
        globalCement += cementToAdd;
        globalTools += toolsToAdd;

        std::cout << "Added resources - Bricks: " << bricksToAdd << ", Cement: " << cementToAdd << ", Tools: " << toolsToAdd << "\n";

        Carts[cartNumber] = false;        

        return true;
    }

    else{
        return false;
    }
  
}

void ResourceManager::useResources(int bricksToUse, int cementToUse, int toolsToUse) {
    // Acquire the global mutex to ensure thread-safe resource usage
    std::lock_guard<std::mutex> lock(globalMutex);

    globalBricks -= bricksToUse;
    globalCement -= cementToUse;
    globalTools -= toolsToUse;

    std::cout << "Used resources - Bricks: " << bricksToUse << ", Cement: " << cementToUse << ", Tools: " << toolsToUse << "\n";
}

void DynamicResourceGenerator::generateResources() {
    // Randomly decide the quantity and types of resources to generate
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> quantityDistribution(1, 10); // Adjust the range as needed

    int bricksToAdd = quantityDistribution(gen);
    int cementToAdd = quantityDistribution(gen);
    int toolsToAdd = quantityDistribution(gen);

    //lock acquired in resources class

    bool ResourcesAdded = ResourceManager().addResources(bricksToAdd, cementToAdd, toolsToAdd);

   if(ResourcesAdded==false)
   {

    std::cout<<"Waiting for cart to be available\n";
    std::unique_lock<std::mutex> lock(CartsMutex);
    check_cart.wait(lock);
    std::cout<<"Cart is available now\n";
    ResourcesAdded = ResourceManager().addResources(bricksToAdd, cementToAdd, toolsToAdd);

   }
}

void TaskAssigner::assignTask(ConstructionTask* task, Worker* workers, int numWorkers) {

    bool taskAssigned=false;
    IOManager io_Manager;

        if(io_Manager.isAdverseWeather())
        {
            std::unique_lock<std::mutex> lock(globalMutex);
            weatherCV.wait(lock, [&io_Manager]{ return !io_Manager.isAdverseWeather(); });
        }



    // Implement task assignment logic based on priority and worker skill
    for (int i = 0; i < numWorkers; ++i) {
        if (!workers[i].onBreak) {
            // Check if the worker has the required skill level for the task
            if (task->priorityLevel <= workers[i].skillLevel) {
                // Acquire the global mutex before modifying shared data structures
                std::lock_guard<std::mutex> lock(WorkersMutex);

                // Assign the task to the worker
                std::cout << "Assigned task '" << task->taskName << "' to worker '" << workers[i].workerName << "'.\n";
                workers[i].PerformTask(task);

                // Add further logic for resource allocation, synchronization, etc.
                break;
            }
        }
        else{
            workers[i].onBreak=false;
        }
    }
}

/*void ConstructionTask::operator()() {
    // Wait for resources to be available
    while (globalBricks < requiredBricks || globalCement < requiredCement || globalTools < requiredTools) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Acquire resources
    ResourceManager().useResources(requiredBricks, requiredCement, requiredTools);

    // Implementation of construction task
    // You can simulate the task by using sleep or any other appropriate function
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // After task completion, simulate weather check
    IOManager().simulateWeatherConditions();
}

void Worker::operator()() {
    // Implementation of worker behavior
    while (true) {
        // Simulate worker working on tasks
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Take a 1-second break after every task
        if (!onBreak) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            onBreak = shouldTakeBreak();
        }

        std::cout << "Worker '" << workerName << "' working.\n";

        // Check for fatigue break (20% chance)
        if (shouldTakeBreak()) {
            onBreak = true;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
*/

bool Worker::PerformTask(ConstructionTask *task)
{
    while(globalBricks < task->requiredBricks || globalCement < task->requiredCement || globalTools < task->requiredTools)
    {
        // Acquire resources using dynamic resource generator, mutex locked in fucntion
        DynamicResourceGenerator().generateResources();
    }

    std::cout << "Worker " << workerName << " is performing task " << task->taskName << "\n"; 
    
    //global mutex locked in relevant functions
    ResourceManager().useResources(task->requiredBricks, task->requiredCement, task->requiredTools);

    this->onBreak=true;
   
    return true;

}

void Building::constructBuilding() {
    // Create a unique lock with defer_lock to manually control locking
    std::unique_lock<std::mutex> lock(globalMutex, std::defer_lock);

    // Iterate through the tasks required to complete the building
    while(tasksCompleted < tasksRequired) {
        
        //adverse conditions simulated in taskAssigner
    }

    std::cout << "Building '" << buildingName << "' construction completed.\n";
    tasksCompleted++;
}



void simulateConstruction(ConstructionTask* tasks, Worker* workers, Building* buildings, int numTasks, int numWorkers, int numBuildings) {
    // Create a PriorityScheduler instance
    PriorityScheduler scheduler;
    TaskAssigner taskAssigner;

    // Add tasks to the scheduler
    for (int i = 0; i < numTasks; ++i) {
        scheduler.addTask(&tasks[i]);
    }

    std::thread workerThreads[numWorkers];
    for (int i = 0; i < numWorkers; ++i) {
        workerThreads[i] = std::thread([&tasks, &workers, numWorkers, &scheduler, &taskAssigner]() {
            while (ConstructionTask* task = scheduler.getNextTask()) {
                taskAssigner.assignTask(task, workers, numWorkers);
            }
        });
    }

    // Start building construction threads
    std::thread buildingThreads[numBuildings];
    for (int i = 0; i < numBuildings; ++i) {
        buildingThreads[i] = std::thread(&Building::constructBuilding, &buildings[i]);
    }

    // Join worker threads
    for (int i = 0; i < numWorkers; ++i) {
        workerThreads[i].join();
    }

    // Join building construction threads
    for (int i = 0; i < numBuildings; ++i) {
        buildingThreads[i].join();
    }
}


int main() {
    // Initialize construction site components
    const int numTasks = 5;
    const int numWorkers = 5;
    const int numBuildings = 10;

    ConstructionTask tasks[numTasks] = {
        ConstructionTask("Laying Bricks", 2, 10, 5, 2),
        ConstructionTask("Mixing Cement", 2, 5, 10, 3),
        ConstructionTask("Scaffolding", 1, 8, 4, 1),
        ConstructionTask("Installing Windows", 3, 3, 6, 2),
        ConstructionTask("Painting", 3, 2, 2, 4)
    };

    Worker workers[numWorkers] = {
        Worker("Worker1", 1),
        Worker("Worker2", 2),
        Worker("Worker3", 2),
        Worker("Worker4", 3),
        Worker("Worker5", 3)
    };

    Building buildings[numBuildings] = {
        Building("Building1", 3),
        Building("Building2", 2),
        Building("Building3", 4),
        Building("Building4", 2),
        Building("Building5", 3),
        Building("Building6", 2),
        Building("Building7", 4),
        Building("Building8", 3),
        Building("Building9", 2),
        Building("Building10", 3)
    };

    simulateConstruction(tasks, workers, buildings, numTasks, numWorkers, numBuildings);

    return 0;
}

void PriorityScheduler::addTask(ConstructionTask *task)
{
    switch (task->priorityLevel) {
        case 1:
            lowPriorityQueue[lowPriorityCount++] = task;
            break;
        case 2:
            mediumPriorityQueue[mediumPriorityCount++] = task;
            break;
        case 3:
            highPriorityQueue[highPriorityCount++] = task;
            break;
        default:
            std::cout << "Invalid priority level.\n";
    }
}

ConstructionTask *PriorityScheduler::getNextTask()
{
    if (highPriorityCount > 0) {
        return highPriorityQueue[--highPriorityCount];
    } else if (mediumPriorityCount > 0) {
        return mediumPriorityQueue[--mediumPriorityCount];
    } else if (lowPriorityCount > 0) {
        return lowPriorityQueue[--lowPriorityCount];
    } else {
        std::cout << "No tasks remaining.\n";
        return nullptr;
    }
}
