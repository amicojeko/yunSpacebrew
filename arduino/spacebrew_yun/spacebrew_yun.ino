#include <Bridge.h>
#include <Console.h>
#include <Process.h>

/**
 *	Arduino Yun Example
 *
 *	This example code is in the public domain.
 *	
 * 	@date 		July 3, 2013
 *  @author		Julio Terra
 *   
 */


 
/**
 * SPACEBREW VARIABLES
 */
enum SBmsg { MSG_START = char(29), MSG_DIV = char(30), MSG_END = char(31) };

struct Publisher {
	char *name;
	char *type;
	char *defaultValue;
	Publisher * next;
};

struct Subscriber{
	char *name;
	char *type;
	Subscriber * next;
};

Process brew;
String name;
String server;
int port;
String description;
Subscriber * subscribers;
Publisher * publishers;
String sub_name = "";
String sub_msg = "";
String sub_type = "";
boolean read_name = false;
boolean read_msg = false;
int sub_name_max = 25;
int sub_msg_max = 50;

// make pid removing process into an object that is declared and instantiated 
// in the set-up method so that these resources are relieved for later.
Process pids;
int const pidLength = 6;
int const sbPidsLen = 4;
char pid [6] = {'\0','\0','\0','\0','\0','\0'};
int sbPids [4] = {-1, -1, -1, -1};


/**
 * APP VARIABLES
 */
int counter = 0;
long last = 0;
int interval = 2000;

void setup() { 
    delay(1000);

    Serial.begin(57600);
	while (!Serial) { 	Serial.println("connecting"); }
	Serial.println("App Started"); 

  	//Initialize Console and wait for port to open:
	Bridge.begin();
	Serial.println("Bridge Started"); 

	spacebrew("sandbox.spacebrew.cc", "aYun", "Arduino Yun spacebrew test");
	addPublish("string test", "string");
	addPublish("range test", "range");
	addPublish("boolean test", "boolean");
	addSubscribe("string test", "string");
	addSubscribe("range test", "range");
	addSubscribe("boolean test", "boolean");
	connect(); 
   
	Console.buffer(64);
//	Serial.println("Console Started"); 
} 


void loop() { 
	if ((millis() - last) > interval) {
		String test_str = "string test";
		String test_ran = "range test";
		String test_bool = "boolean test";
		String test_msg = "testing, testing, ";
		test_msg += counter;
		counter ++;

		boolean test_flag = true;
		send(test_str, test_msg);
		send(test_ran, 500);
		send(test_bool, true);

		last = millis();

	}

	monitor();
} 


/***********************
 ** SPACEBREW METHODS **
 ***********************/

void spacebrew(const String _server, const String _name, const String _description) {
	name = _name;
	server = _server;
	description = _description;
	port = 9000;
}

void addPublish(const String name, String type) {
	Publisher *p = new Publisher();
	p->name = createString(name.length() + 1);
	p->type = createString(type.length() + 1);
	name.toCharArray(p->name, name.length() + 1);
	type.toCharArray(p->type, type.length() + 1);

	if (publishers == NULL){
		publishers = p;
	} else {
		Publisher *curr = publishers;
		while(curr->next != NULL){
			curr = curr->next;
		}
		curr->next = p;
	}
}

void addPublish(char * name, char * type) {
	addPublish( String(name), String(type) );
}

void addSubscribe(const String name, String type) {
	Subscriber *p = new Subscriber();
	p->name = createString(name.length() + 1);
	p->type = createString(type.length() + 1);
	name.toCharArray(p->name, name.length() + 1);
	type.toCharArray(p->type, type.length() + 1);

	if (subscribers == NULL){
		subscribers = p;
	} 

	else {
		Subscriber *curr = subscribers;
		while(curr->next != NULL){
			curr = curr->next;
		}
		curr->next = p;
	}
}

void addSubscribe(char * name, char * type) {
	addSubscribe( String(name), String(type) );
}

void connect() {

	killBrewPids();

 	brew.begin("python"); // Process should launch the "curl" command
	brew.addParameter("/usr/lib/python2.7/spacebrew.py"); // Process should launch the "curl" command
	brew.addParameter("--server");
	brew.addParameter(server);
	brew.addParameter("--port");
	brew.addParameter(String(port));
	brew.addParameter("-n");
	brew.addParameter(name);
	brew.addParameter("-d");
	brew.addParameter(description);

	if (subscribers != NULL) {
		Subscriber *curr = subscribers;
		while(curr != NULL){
			Serial.print("sub name: ");
			Serial.println(curr->name);

			brew.addParameter("-s"); // Add the URL parameter to "curl"
			brew.addParameter(curr->name); // Add the URL parameter to "curl"
			brew.addParameter(","); // Add the URL parameter to "curl"
			brew.addParameter(curr->type); // Add the URL parameter to "curl"

			if (curr->next == NULL) curr = NULL;
			else curr = curr->next;
		}
	}
	if (publishers != NULL) {
		Publisher *curr = publishers;
		while(curr != NULL){
			Serial.print("pub name: ");
			Serial.println(curr->name);

			brew.addParameter("-p"); // Add the URL parameter to "curl"
			brew.addParameter(curr->name); // Add the URL parameter to "curl"
			brew.addParameter(","); // Add the URL parameter to "curl"
			brew.addParameter(curr->type); // Add the URL parameter to "curl"

			if (curr->next == NULL) curr = NULL;
			else curr = curr->next;
		}
	}

    Serial.println("connect - starting console");

	Console.begin();
	brew.runAsynchronously();
	while (!Console) { ; }

    Serial.println("connect - connected to spacebrew.py script");
}

void monitor() {
	while (Console.available() > 0) {
	    char c = Console.read();
	    if (c == char(MSG_START)) {
	    	read_name = true;
	    } else if (c == char(MSG_DIV) || sub_name.length() > sub_name_max) {
	    	read_name = false;
	    	read_msg = true;
	    } else if (c == char(MSG_END) || sub_msg.length() > sub_msg_max) {
	    	read_msg = false;
	    	onMessage();
	    } else {
			if (read_name == true) {
				sub_name += c;
			} else if (read_msg == true) {
				sub_msg += c;
			} else {
			    Serial.print(c);
			}	    	
	    }
	}	
}

void onMessage() {
	if (subscribers != NULL) {
		Subscriber *curr = subscribers;
		while((curr != NULL) && (sub_type == "")){
			if (sub_name.equals(curr->name) == true) {
				sub_type = curr->type;
			}
			if (curr->next == NULL) curr = NULL;
			else curr = curr->next;
		}
	}

	if ( sub_type.equals("range") ) {
		onRangeMessage( sub_name, int(sub_msg.toInt()) );
	} else if ( sub_type.equals("boolean") ) {
		onBooleanMessage( sub_name, ( sub_msg.equals("false") ? false : true ) );
	} else if ( sub_type.equals("string") ) {
		onStringMessage( sub_name, sub_msg );
	} else {
		onCustomMessage( sub_name, sub_msg, sub_type );
	}

	sub_name = "";
	sub_msg = "";
	sub_type = "";
}

void onRangeMessage( String name, int value ) {
	Serial.print( "Range message on publisher: '" );		
	Serial.print( name );		
	Serial.print( "' - msg: " );		
	Serial.println( value );		
}

void onBooleanMessage( String name, boolean value ) {
	Serial.print( "Boolean message on publisher: '" ) ;		
	Serial.print( name);		
	Serial.print( "' - msg: " );		 
	Serial.println( (value ? "true" : "false") );		
}

void onStringMessage( String name, String value ) {
	Serial.print( "String message on publisher: '") ;		
	Serial.print( name );		
	Serial.print( "' - msg: " );		
	Serial.println( value );		
}

void onCustomMessage( String name, String value, String type) {
	Serial.print( "Custom message on publisher: '") ;		
	Serial.print( name );		
	Serial.print( "', of type: '" );		
	Serial.print( type );		
	Serial.print( "' - msg: " );		
	Serial.println( value );		
}


boolean send(const String name, const String value){
	Console.print(char(29));
	Console.print(name);
	Console.print(char(30));
	Console.print(value);
	Console.print(char(31));
	Console.flush();
	return true;
}

boolean send(const String name, bool value){
	return send(name, (value ? "true" : "false"));
}

boolean send(const String name, int value) {
	return send(name, String(value));
}

bool send(const String name, long value) {
	return send(name, String(value));
}

bool send(const String name, float value) {
	return send(name, String(value));
}


/**
 * method that gets the pid of all spacebrew.py instances running on the linino.
 */
void getSbPid() {

	// request the pid of all python processes
	pids.begin("python");
	pids.addParameter("/usr/lib/python2.7/getSbPid.py"); // Process should launch the "curl" command
	pids.run();

	Serial.println("getSbPid - process running");

	int sbPidsIndex = 0;
    int pidCharIndex = 0;
    char c = '\0';

	while ( pids.available() > 0 ) {

	    c = pids.read();

		if ( c >= '0' && c <= '9' ) {
			pid[pidCharIndex] = c;
			pidCharIndex = (pidCharIndex + 1) % pidLength;
		} 

		else if ( (c == ' ' || c == '\n') && pidCharIndex > 0) {
			sbPids[sbPidsIndex] = atoi(pid);
			if ( sbPidsIndex < (sbPidsLen - 1) ) sbPidsIndex = (sbPidsIndex + 1);    		

			for( int i = 0; i < pidLength; i++ ){ 
				pid[i] = '\0';
				pidCharIndex = 0;
			}
		}
	}

	// print out the pid of all python processes
	Serial.println("\nSB pids recap: ");
	for (int i = 0; i < sbPidsIndex; i++) {
		Serial.print(i);
		Serial.print(" : ");
		Serial.println(sbPids[i]);                
	}
}

/**
 * method that kills all of the spacebrew.py instances that are running 
 * on the linino.
 */
void killBrewPids() {
	getSbPid();
	delay(400);

	for (int i = 0; i < sbPidsLen; i ++) {
		if (sbPids[i] > 0) {
			char * newPID = itoa(sbPids[i], pid, 10);
			Serial.print("deleting pid: ");
			Serial.println(newPID);

			Process p;
			p.begin("kill");
			p.addParameter("-9");
			p.addParameter(newPID);		// Process should launch the "curl" command
			p.run();            		// Run the process and wait for its termination	

			delay(400);						
		}
	}
}

static char *cloneString(char *s){
	int n = strlen(s);
	char *out = (char *)malloc(n+1);//new char[n];
	strcpy(out, s);
	return out;
}

static char *createString(int n){
	char *out = (char *)malloc(n+1);//new char[n];
	return out;
}

