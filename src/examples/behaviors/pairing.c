#include <kilolib.h>

#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3
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
int msg = -1;
int id = 0;
int pairedID = UNPAIRED;
int msg_state = -1;
int self_state = UNPAIRED;

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
      current_motion = new_motion;
        
      if (current_motion == STOP)
      {
         set_motors(0, 0);
      }
      else if (current_motion == FORWARD)
      {
         spinup_motors();
         set_motors(kilo_straight_left, kilo_straight_right);
      }
      else if (current_motion == LEFT)
      {
         spinup_motors();
         set_motors(kilo_turn_left, 0);
      }
      else if (current_motion == RIGHT)
      {
         spinup_motors();
         set_motors(0, kilo_turn_right);
      }
   }
}

void loop()
{
   if (kilo_ticks > last_motion_update + 32)
   {
      last_motion_update = kilo_ticks;
 
      // If message is from pair
      if (msg == pairedID)
      {
         set_color(RGB(1, 1, 1));
         set_motion(STOP);
         message.data[0] = pairedID;
         message.data[1] = PAIRED;
         message.crc = message_crc(&message);
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
           
         // Generate an 8-bit random number (between 0 and 2^8 - 1 = 255).
         int random_number = rand_hard();
            
         // Compute the remainder of random_number when divided by 4.
         // This gives a new random number in the set {0, 1, 2, 3}.
         int random_direction = (random_number % 4);
            
         // There is a 50% chance of random_direction being 0 OR 1, in which
         // case set the LED green and move forward.
         if ((random_direction == 0) || (random_direction == 1))
         {
            set_color(RGB(0, 1, 0));
            set_motion(FORWARD);
         }
         // There is a 25% chance of random_direction being 2, in which case
         // set the LED red and move left.
         else if (random_direction == 2)
         {
            set_color(RGB(1, 0, 0));
            set_motion(LEFT);
         }
         // There is a 25% chance of random_direction being 3, in which case
         // set the LED blue and move right.
         else if (random_direction == 3)
         {
            set_color(RGB(0, 0, 1));
            set_motion(RIGHT);
         }
      }
      msg = -1;
      msg_state = -1;
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
    
   distance = estimate_distance(distance_measurement);
   if (msg == pairedID)
   {
     //Do nothing - can break pairing otherwise
   } else
   {
     msg = message->data[0];
     msg_state = message->data[1];
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
