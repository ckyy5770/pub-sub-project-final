  all: Publisher Subscriber Center

  Publisher: 
	clang++ -g -Wall -o publisher ./src/SimplePublisher.cpp ./main/mainPub.cpp -lzmq -lpthread -std=c++14
  Subscriber: 
	clang++ -g -Wall -o subscriber ./src/SimpleSubscriber.cpp ./main/mainSub.cpp -lzmq -lpthread -std=c++14
  Center: 
	clang++ -g -Wall -o center ./src/SimpleCenter.cpp ./main/mainCenter.cpp -lzmq -lpthread -std=c++14

  clean:
	$(RM) publisher subscriber center