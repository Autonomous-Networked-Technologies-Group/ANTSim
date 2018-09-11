#include <kilolib.h>

#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3
#define REVERSE 4
#define RANDOM 5
#define UNPAIRED 0
#define PAIRED 1
#define PAIRING -3

message_t message;
// Flag to keep track of message transmission.
int message_sent = 0;
int current_motion = STOP;
int new_message = 0;
message_t message;
uint32_t last_motion_update = 0;
int distance = 0;
int last_distance = 0;
int msg = -1;
int difference = 0;
int id = 0;
int recieved_paired_message = 0;
int pairedID = UNPAIRED;
int msg_state = -1;
int self_state = UNPAIRED;
int last_turn = LEFT;

void setup()
{
   // Initialize message:
   // The type is always NORMAL.
   message.type = NORMAL;
   // Some dummy data as an example.
   id = (rand_hard() % 125) + 2;
   message.data[0] = id;
   message.data[1] = UNPAIRED;
   // It's important that the CRC is computed after the data has been set;
   // otherwise it would be wrong and the message would be dropped by the
   // receiver.
   message.crc = message_crc(&message);
}

// Function to handle motion.
void set_motion(int new_motion)
{
   // Only take an an action if the motion is being changed.
   if (current_motion != new_motion)
   {    
	  if (current_motion == STOP)
	  {
		  spinup_motors();
	  }
      if (new_motion == STOP)
      {
         set_motors(0, 0);
         set_color(RGB(0, 0, 0));
      }
      else if (new_motion == FORWARD)
      {
         //spinup_motors();
         set_motors(kilo_straight_left, kilo_straight_right);
         set_color(RGB(0, 1, 0));
      }
      else if (new_motion == LEFT)
      {
         //spinup_motors();
         set_motors(kilo_turn_left, 0);
         set_color(RGB(1, 0, 0));
         last_turn = LEFT;
      }
      else if (new_motion == RIGHT)
      {
         //spinup_motors();
         set_motors(0, kilo_turn_right);
         set_color(RGB(0, 0, 1));
         last_turn = RIGHT;
      }
      else if (new_motion == REVERSE)
      {
	     if (current_motion == LEFT)
	     {
			 set_motion(RIGHT);
		 }
		 else if (current_motion == RIGHT)
		 {
			 set_motion(LEFT);
		 }
		 else if (current_motion == STOP)
		 {
			 set_motion(FORWARD);
		 }
		 else if (current_motion == FORWARD)
		 {
             //int random_direction = (rand_hard() % 2);
             if (last_turn == LEFT)
             {
                 set_motion(RIGHT);
                 //delay(4000);
             }
             else if (last_turn == RIGHT)
             {
                 set_motion(LEFT);
                 //delay(4000);
             }
		 }
      }
      else if (new_motion == RANDOM)
      {
		 // Compute the remainder of random_number when divided by 4.
         // This gives a new random number in the set {0, 1, 2, 3}.
         int random_direction = (rand_hard() % 4);
            
         // There is a 50% chance of random_direction being 0 OR 1, in which
         // case set the LED green and move forward.
         if ((random_direction == 0) || (random_direction == 1))
         {
            set_motion(FORWARD);
         }
         // There is a 25% chance of random_direction being 2, in which case
         // set the LED red and move left.
         else if (random_direction == 2)
         {
            set_motion(LEFT);
         }
         // There is a 25% chance of random_direction being 3, in which case
         // set the LED blue and move right.
         else if (random_direction == 3)
         {
            set_motion(RIGHT);
         }
	  }
	 if (new_motion != RANDOM)// && new_motion != REVERSE)
	 {
       current_motion = new_motion; 
     }
   }
}

void loop()
{
   if (kilo_ticks > last_motion_update + 16)
   {
      last_motion_update = kilo_ticks;
 
      // If message is from pair
      if (msg == pairedID)
      {
         if (difference < 0)
         {
           set_motion(REVERSE);
         } else
         {
			 if (rand_hard() % 3 == 0)
			 {
			   set_motion(RANDOM);
		     }
		 }
         message.data[0] = pairedID;
         message.data[1] = PAIRED;
         message.crc = message_crc(&message);
         set_color(RGB(1, 1, 1));
         
      }
      // If message is from unpaired and self is unpaired
      else if (msg_state == UNPAIRED && pairedID == UNPAIRED)
      {
         set_color(RGB(0, 1, 1));
         set_motion(STOP);
         pairedID = msg + id;
         message.data[0] = pairedID;
         message.data[1] = id;
         message.crc = message_crc(&message);
      }
      // If message is attempting to establish pair and self is unpaired
      else if (msg_state >= 2 && pairedID == UNPAIRED)
      {
         set_color(RGB(1, 1, 0)); //Yellow
         set_motion(STOP);
         pairedID = msg_state + id;
         message.data[0] = pairedID;
         message.data[1] = PAIRED;
         message.crc = message_crc(&message);
      }
      // No significant messages - keep moving
      else 
      {
         message.data[0] = id;
         message.data[1] = UNPAIRED;
         message.crc = message_crc(&message);
         pairedID = UNPAIRED;
         if (rand_hard() % 5 == 0)
	     {
		   set_motion(RANDOM);
	     }  
      }
      msg = -1;
      msg_state = -1;
      difference = 0;
   }
}

message_t *message_tx()
{
   return &message;
}

void message_tx_success()
{
   // Set the flag on message transmission.
   message_sent = 1;
}

void message_rx(message_t *message, distance_measurement_t *distance_measurement)
{
   // Set flag on message reception.
   new_message = 1;
    
   if (msg == pairedID)
   {
     distance = estimate_distance(distance_measurement);
     difference += last_distance - distance;
     last_distance = distance;
   } else
   {
     msg = message->data[0];
     msg_state = message->data[1];
     distance = estimate_distance(distance_measurement);
   }
}

int main()
{
   kilo_init();
   // Register the message_tx callback function.
   kilo_message_tx = message_tx;
   // Register the message_tx_success callback function.
   kilo_message_tx_success = message_tx_success;
   // Register the message_rx callback function.
   kilo_message_rx = message_rx;
   kilo_start(setup, loop);
    
   return 0;
}
