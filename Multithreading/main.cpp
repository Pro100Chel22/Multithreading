#include <queue>
#include <thread>
#include <mutex>
#include <iostream>
#include <Windows.h>
#include <csignal>
#include <string>
#include <random>

using namespace std;

void signalHandler(int signal) { }

struct Application
{
	int groupId;
	char priority; 

	bool operator<(const Application& other) const
	{
		return priority < other.priority;
	}
};

class ServiceSystem
{
	bool active;
	int minMsGenerator, maxMsGenerator;
	int minMsDevice, maxMsDevice;
	int sizeOfQueue;
	priority_queue<Application> applications;
	mutex order;
	thread threadOfGenerator;
	vector<thread> threadsOfDevices;
	pair<bool, Application> statusOfGenerator;
	vector<vector<pair<bool, char>>> statusOfDevices;

	void push(Application application)
	{
		lock_guard<mutex> write(order);
		applications.push(application);
	}
	Application getByGroupIdTop(int groupId)
	{
		lock_guard<mutex> get(order);
		if(!applications.empty())
		{
			Application top = applications.top();
			if (top.groupId == groupId)
			{
				applications.pop();
				return top;
			}
		}
		return Application{ -1, -1 };
	}

	void generator(pair<bool, Application>& status)
	{
		while (isActive())
		{
			if (applications.size() < sizeOfQueue)
			{
				random_device rd;
				mt19937 gen(rd());

				uniform_int_distribution<int> groupIdDis(0, statusOfDevices.size() - 1);
				uniform_int_distribution<int> priorityDis(1, 3);
				uniform_int_distribution<int> timeDis(minMsGenerator, maxMsGenerator);

				Application newApplication{ groupIdDis(gen), priorityDis(gen) };

				this->push(newApplication);

				status.first = true;
				status.second = newApplication;
				this_thread::sleep_for(chrono::milliseconds(timeDis(gen)));
			}
			else
			{
				status.first = false;
			}
		}
	}
	void device(int groupId, int deviceId, pair<bool, char>& status)
	{
		while (isActive())
		{
			Application app = getByGroupIdTop(groupId);
			if (app.groupId == groupId)
			{
				random_device rd;
				mt19937 gen(rd());
				uniform_int_distribution<int> timeDis(minMsDevice, maxMsDevice);

				status.first = true;
				status.second = app.priority;

				this_thread::sleep_for(chrono::milliseconds(timeDis(gen)));
			}
			else
			{
				status.first = false;
			}
		}
	}

public:
	ServiceSystem(int countOfGroup, int sizeOfQueue, vector<int> countOfDevices, int minMsGen, int maxMsGen, int minMsDev, int maxMsDev)
	{
		this->active = true;
		this->sizeOfQueue = sizeOfQueue;
	
		this->threadOfGenerator = thread(&ServiceSystem::generator, this, ref(statusOfGenerator));
		this->statusOfDevices = vector<vector<pair<bool, char>>>(countOfGroup);
		for (int i = 0; i < countOfGroup; i++) 
		{
			this->statusOfDevices[i] = vector<pair<bool, char>>(countOfDevices[i]);
			for (int j = 0; j < countOfDevices[i]; j++)
			{
				this->threadsOfDevices.push_back(thread(&ServiceSystem::device, this, i, j, ref(statusOfDevices[i][j])));
			}
		}

		this->minMsGenerator = minMsGen;
		this->maxMsGenerator = maxMsGen;
		this->minMsDevice = minMsDev;
		this->maxMsDevice = maxMsDev;

		if (minMsGenerator < 50) minMsGenerator = 50;
		if (minMsDevice < 50) minMsDevice = 50;
	}
	~ServiceSystem()
	{
		threadOfGenerator.detach();

		for (int i = 0; i < threadsOfDevices.size(); i++)
		{
			threadsOfDevices[i].detach();
		}
	}

	bool isActive()
	{
		return active;
	}
	void deactivate()
	{
		active = false;
	}

	void showInformation()
	{
		system("cls");
		printf("System status: %s\n", ((active) ? "active" : "inactive"));

		if (active)
		{
			printf("Applications in the queue %d out of %d\n", (int)this->applications.size(), this->sizeOfQueue);

			bool status = statusOfGenerator.first;
			string groupId = to_string(statusOfGenerator.second.groupId + 1);
			string priority = to_string(statusOfGenerator.second.priority);
			printf("Generator %s\n", ((status) ? ("active; the last generated application group id: " + groupId + " priority: " + priority).c_str() : "inactive"));

			for (int i = 0; i < statusOfDevices.size(); i++)
			{
				printf("Group %d devices:\n", i + 1);
				for (int j = 0; j < statusOfDevices[i].size(); j++)
				{
					status = statusOfDevices[i][j].first;
					priority = to_string(int(statusOfDevices[i][j].second));
					printf("\tDevice %d %s\n", j + 1, ((status) ? (" active; priority of application: " + priority).c_str() : " inactive"));
				}
			}
		}

		this_thread::sleep_for(chrono::milliseconds(40));
	}
};

int main() 
{
	signal(SIGINT, signalHandler);

	int countOfGroup;
	cout << "Enter the number of groups (minimum quantity 3):\n";
	cin >> countOfGroup;
	if (countOfGroup < 3) countOfGroup = 3;

	vector<int> countOfDevices(countOfGroup, 4);
	cout << "Enter the number of devices for each group separated by a space (minimum quantity 4):\n";
	for (int i = 0; i < countOfGroup; i++)
	{
		cin >> countOfDevices[i];
		if (countOfDevices[i] < 4) countOfDevices[i] = 4;
	}

	int sizeOfQueue;
	cout << "Enter the queue size: \n";
	cin >> sizeOfQueue;

	int minMsGenerator, maxMsGenerator;
	cout << "Enter the minimum and maximum time for the generator in milliseconds (minimum quantity 50):\n";
	cin >> minMsGenerator >> maxMsGenerator;

	int minMsDevice, maxMsDevice ;
	cout << "Enter the minimum and maximum time for the generator in milliseconds (minimum quantity 50):\n";
	cin >> minMsDevice >> maxMsDevice;

	ServiceSystem System(countOfGroup, sizeOfQueue, countOfDevices, minMsGenerator, maxMsGenerator, minMsDevice, maxMsDevice);

	while (System.isActive())
	{
		if(GetAsyncKeyState(VK_CONTROL) & 0x8000 and GetAsyncKeyState('C') & 0x8000 || GetAsyncKeyState(VK_ESCAPE))
		{
			System.deactivate();
		}

		System.showInformation();
	}

	system("pause");

	return 0;
}