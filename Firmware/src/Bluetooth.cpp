/*
 * Bluetooth.cpp
 *
 *  Created on: 2015. dec. 21.
 *      Author: xdever
 */

#include "Bluetooth.h"
#include <cstring>
#include <helpers.h>

#define DEFAULT_IO_CAPABILITY          (icNoInputNoOutput)
#define DEFAULT_MITM_PROTECTION                  (FALSE)

Bluetooth::Bluetooth(const char *i_name) {
	Connected=FALSE;
	SPPServerSDPHandle=0;
	bluetoothStackID=0;
	ServerPortID=0;
	OOBSupport=FALSE;
	MITMProtection=DEFAULT_MITM_PROTECTION;
	IOCapability     = DEFAULT_IO_CAPABILITY;
	OOBSupport       = FALSE;

	this->name = i_name;
}

Bluetooth::~Bluetooth() {

}

int Bluetooth::displayCallback(int length, char *message){
	UNUSED(length);
	UNUSED(message);
    return TRUE;
}

/* The following function is used to initialize the application      */
  /* instance.  This function should open the stack and prepare to     */
  /* execute commands based on user input.  The first parameter passed */
  /* to this function is the HCI Driver Information that will be used  */
  /* when opening the stack and the second parameter is used to pass   */
  /* parameters to BTPS_Init.  This function returns the               */
  /* BluetoothStackID returned from BSC_Initialize on success or a     */
  /* negative error code (of the form APPLICATION_ERROR_XXX).          */
int Bluetooth::initializeApplication()
{
  int ret_val = -1;

  /* Initiailize some defaults.                                        */
  /*SerialPortID           = 0;
  UI_Mode                = UI_MODE_SELECT;
  LoopbackActive         = FALSE;
  DisplayRawData         = FALSE;
  AutomaticReadActive    = FALSE;
  NumberofValidResponses = 0;*/

 /* Try to Open the stack and check if it was successful.          */
 if(!openStack()){
 	/* The stack was opened successfully.  Now set some defaults.  */

	/* First, attempt to set the Device to be Connectable.         */
	ret_val = setConnectabilityMode(cmConnectableMode);

	/* Next, check to see if the Device was successfully made      */
	/* Connectable.                                                */
	if(!ret_val)
	{
	   /* Now that the device is Connectable attempt to make it    */
	   /* Discoverable.                                            */
	   ret_val = setDiscoverabilityMode(dmGeneralDiscoverableMode);

	   /* Next, check to see if the Device was successfully made   */
	   /* Discoverable.                                            */
	   if(!ret_val)
	   {
		  /* Now that the device is discoverable attempt to make it*/
		  /* pairable.                                             */
		  ret_val = setPairable();
		  if(!ret_val)
		  {
			 /* Attempt to register a HCI Event Callback.          */
			 ret_val = HCI_Register_Event_Callback(bluetoothStackID, HCI_Event_Callback, (unsigned long)this);
			 if(ret_val > 0)
			 {
				/* Assign a NULL BD_ADDR for comparison.           */
				ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

				/* Return success to the caller.                   */
				ret_val = (int)bluetoothStackID;
			 }

		  }

	   }

	}

	/* In some error occurred then close the stack.                */
	if(ret_val < 0)
	{
	   /* Close the Bluetooth Stack.                               */
	   closeStack();
	}
 }
 else
 {
	/* There was an error while attempting to open the Stack.      */
	return -1;
 }

 return ret_val;
}

void BTPSAPI Bluetooth::GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter){
	((Bluetooth *)CallbackParameter)->gapEventCallback(BluetoothStackID, GAP_Event_Data);
}

void BTPSAPI Bluetooth::HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter){
	((Bluetooth *)CallbackParameter)->hciEventCallback(BluetoothStackID, HCI_Event_Data);
}


void BTPSAPI Bluetooth::SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter)
{
	((Bluetooth *)CallbackParameter)->sppEventCallback(BluetoothStackID, SPP_Event_Data);
}


void Bluetooth::sppEventCallback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data){
	UNUSED(BluetoothStackID);
	UNUSED(SPP_Event_Data);
}


void Bluetooth::gapEventCallback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data){
	UNUSED(BluetoothStackID);
	UNUSED(GAP_Event_Data);


	int                               Result;
	int                               Index;
	BD_ADDR_t                         NULL_BD_ADDR;
	GAP_Authentication_Information_t  GAP_Authentication_Information;

	/* First, check to see if the required parameters appear to be       */
	/* semi-valid.                                                       */
	if((BluetoothStackID) && (GAP_Event_Data))
	{
	  /* The parameters appear to be semi-valid, now check to see what  */
	  /* type the incoming event is.                                    */
	  switch(GAP_Event_Data->Event_Data_Type)
	  {
		 case etInquiry_Result:
			break;
		 case etInquiry_Entry_Result:
			break;
		 case etAuthentication:
			/* An authentication event occurred, determine which type of*/
			/* authentication event occurred.                           */
			switch(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type)
			{
			   case atLinkKeyRequest:
				  /* Setup the authentication information response      */
				  /* structure.                                         */
				  GAP_Authentication_Information.GAP_Authentication_Type    = atLinkKey;
				  GAP_Authentication_Information.Authentication_Data_Length = 0;

				  /* See if we have stored a Link Key for the specified */
				  /* device.                                            */
				  for(Index=0;Index<(int)(sizeof(linkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
				  {
					 if(COMPARE_BD_ADDR(linkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
					 {
						/* Link Key information stored, go ahead and    */
						/* respond with the stored Link Key.            */
						GAP_Authentication_Information.Authentication_Data_Length   = sizeof(Link_Key_t);
						GAP_Authentication_Information.Authentication_Data.Link_Key = linkKeyInfo[Index].LinkKey;

						break;
					 }
				  }

				  /* Submit the authentication response.                */
				  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

				  break;
			   case atPINCodeRequest:
				  /* Note the current Remote BD_ADDR that is requesting */
				  /* the PIN Code.                                      */
				  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

				  /* Inform the user that they will need to respond with*/
				  /* a PIN Code Response.                               */
				  break;
			   case atAuthenticationStatus:
				  /* Flag that there is no longer a current             */
				  /* Authentication procedure in progress.              */
				  ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
				  break;
			   case atLinkKeyCreation:
				  /* Now store the link Key in either a free location OR*/
				  /* over the old key location.                         */
				  ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

				  for(Index=0,Result=-1;Index<(int)(sizeof(linkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
				  {
					 if(COMPARE_BD_ADDR(linkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
						break;
					 else
					 {
						if((Result == (-1)) && (COMPARE_BD_ADDR(linkKeyInfo[Index].BD_ADDR, NULL_BD_ADDR)))
						   Result = Index;
					 }
				  }

				  /* If we didn't find a match, see if we found an empty*/
				  /* location.                                          */
				  if(Index == (sizeof(linkKeyInfo)/sizeof(LinkKeyInfo_t)))
					 Index = Result;

				  /* Check to see if we found a location to store the   */
				  /* Link Key information into.                         */
				  if(Index != (-1))
				  {
					 linkKeyInfo[Index].BD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
					 linkKeyInfo[Index].LinkKey = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key;
				  }
				  break;
			   case atIOCapabilityRequest:
				  /* Setup the Authentication Information Response      */
				  /* structure.                                         */
				  GAP_Authentication_Information.GAP_Authentication_Type                                      = atIOCapabilities;
				  GAP_Authentication_Information.Authentication_Data_Length                                   = sizeof(GAP_IO_Capabilities_t);
				  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.IO_Capability            = (GAP_IO_Capability_t)IOCapability;
				  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.MITM_Protection_Required = MITMProtection;
				  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.OOB_Data_Present         = OOBSupport;

				  /* Submit the Authentication Response.                */
				  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);
				  break;
			   case atIOCapabilityResponse:
				   break;
			   case atUserConfirmationRequest:
				  CurrentRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

				  if(IOCapability != icDisplayYesNo)
				  {
					 /* Invoke JUST Works Process...                    */
					 GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
					 GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
					 GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

					 /* Submit the Authentication Response.             */
					 Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

					 /* Flag that there is no longer a current          */
					 /* Authentication procedure in progress.           */
					 ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
				  }
				  break;
			   case atPasskeyRequest:
				  break;
			   case atRemoteOutOfBandDataRequest:
				  /* This application does not support OOB data so      */
				  /* respond with a data length of Zero to force a      */
				  /* negative reply.                                    */
				  GAP_Authentication_Information.GAP_Authentication_Type    = atOutOfBandData;
				  GAP_Authentication_Information.Authentication_Data_Length = 0;

				  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);
				  break;
			   case atPasskeyNotification:
				  break;
			   case atKeypressNotification:
				  break;
			   default:
				  break;
			}
			break;
		 case etRemote_Name_Result:
			break;
		 case etEncryption_Change_Result:
			break;
		 default:
			break;
	  }
	}

}

void Bluetooth::hciEventCallback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data){
	UNUSED(BluetoothStackID);
	UNUSED(HCI_Event_Data);
}

/* The following function is responsible for placing the local       */
/* Bluetooth device into Pairable mode.  Once in this mode the device*/
/* will response to pairing requests from other Bluetooth devices.   */
/* This function returns zero on successful execution and a negative */
/* value on all errors.                                              */
int Bluetooth::setPairable()
{
	int Result;
	int ret_val = 0;

	/* First, check that a valid Bluetooth Stack ID exists.              */
	if(bluetoothStackID)
	{
		/* Attempt to set the attached device to be pairable.             */
		Result = setPairabilityMode(pmPairableMode);

		/* Next, check the return value of the GAP Set Pairability mode   */
		/* command for successful execution.                              */
		if(!Result){
			/* The device has been set to pairable mode, now register an   */
			/* Authentication Callback to handle the Authentication events */
			/* if required.                                                */
			Result = GAP_Register_Remote_Authentication(bluetoothStackID, GAP_Event_Callback, (unsigned long)this);

			/* Next, check the return value of the GAP Register Remote     */
			/* Authentication command for successful execution.            */
			if(Result){
				/* An error occurred while trying to execute this function. */
				ret_val = Result;
			}
		}
		else{
			/* An error occurred while trying to make the device pairable. */
			ret_val = Result;
		}
	} else {
		/* No valid Bluetooth Stack ID exists.                            */
		ret_val = -1;
	}

	return(ret_val);
}


/* The following function is a utility function that exists to delete*/
/* the specified Link Key from the Local Bluetooth Device.  If a NULL*/
/* Bluetooth Device Address is specified, then all Link Keys will be */
/* deleted.                                                          */
int Bluetooth::deleteLinkKey(BD_ADDR_t BD_ADDR){
	int       result;
	Byte_t    Status_Result;
	Word_t    Num_Keys_Deleted = 0;
	BD_ADDR_t NULL_BD_ADDR;

	result = HCI_Delete_Stored_Link_Key(bluetoothStackID, BD_ADDR, TRUE, &Status_Result, &Num_Keys_Deleted);

	/* Any stored link keys for the specified address (or all) have been */
	/* deleted from the chip.  Now, let's make sure that our stored Link */
	/* Key Array is in sync with these changes.                          */

	/* First check to see all Link Keys were deleted.                    */
	ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

	if(COMPARE_BD_ADDR(BD_ADDR, NULL_BD_ADDR))
	   BTPS_MemInitialize(linkKeyInfo, 0, sizeof(linkKeyInfo));
	else
	{
	   /* Individual Link Key.  Go ahead and see if know about the entry */
	   /* in the list.                                                   */
	   for(result=0;(result<(int)(sizeof(linkKeyInfo)/sizeof(LinkKeyInfo_t)));result++)
	   {
		  if(COMPARE_BD_ADDR(BD_ADDR, linkKeyInfo[result].BD_ADDR))
		  {
			 linkKeyInfo[result].BD_ADDR = NULL_BD_ADDR;
			 break;
		  }
	   }
	}

	return(result);
}

/* The following function is responsible for opening the SS1         */
/* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
/* HCI Driver Information structure that contains the HCI Driver     */
/* Transport Information.  This function returns zero on successful  */
/* execution and a negative value on all errors.                     */
int Bluetooth::openStack(){
	int                        result;
	Byte_t                     status;
	BD_ADDR_t                  bd_addr;
	L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

	/* First check to see if the Stack has already been opened.          */
	if(!bluetoothStackID)
	{
	  /* Initialize BTPSKNRl.                                        */
	  BTPS_Init(&BTPS_Initialization);

	  /* Initialize the Stack                                        */
	  result = BSC_Initialize(&HCI_DriverInformation, 0);

	  /* Next, check the return value of the initialization to see if*/
	  /* it was successful.                                          */
	  if(result > 0)
	  {

		 /* The Stack was initialized successfully, inform the user  */
		 /* and set the return value of the initialization function  */
		 /* to the Bluetooth Stack ID.                               */
		 bluetoothStackID = result;

		 /* Initialize the default Secure Simple Pairing parameters. */
		 IOCapability     = DEFAULT_IO_CAPABILITY;
		 OOBSupport       = FALSE;
		 MITMProtection   = DEFAULT_MITM_PROTECTION;

		 /* Set the Name for the initial use.              */
		 GAP_Set_Local_Device_Name (bluetoothStackID, const_cast<char *>(name));

		 /* Go ahead and allow Master/Slave Role Switch.             */
		 L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
		 L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

		 L2CA_Set_Link_Connection_Configuration(bluetoothStackID, &L2CA_Link_Connect_Params);

		 if(HCI_Command_Supported(bluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
			HCI_Write_Default_Link_Policy_Settings(bluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &status);

		 /* Delete all Stored Link Keys.                             */
		 ASSIGN_BD_ADDR(bd_addr, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

		 deleteLinkKey(bd_addr);
	  } else
		  return -1;
	}

	return 0;
}

/* The following function is responsible for closing the SS1         */
/* Bluetooth Protocol Stack.  This function requires that the        */
/* Bluetooth Protocol stack previously have been initialized via the */
/* OpenStack() function.  This function returns zero on successful   */
/* execution and a negative value on all errors.                     */
int Bluetooth::closeStack()
{
	int ret_val = 0;

	/* First check to see if the Stack has been opened.                  */
	if(bluetoothStackID)
	{
		/* Simply close the Stack                                         */
		BSC_Shutdown(bluetoothStackID);

		/* Free BTPSKRNL allocated memory.                                */
		BTPS_DeInit();

		/* Flag that the Stack is no longer initialized.                  */
		bluetoothStackID = 0;

		/* Flag success to the caller.                                    */
		ret_val          = 0;
	}
	else {
		/* A valid Stack ID does not exist, inform to user.               */
		ret_val = -1;
	}

	return(ret_val);
}

/* The following function is responsible for opening a Serial Port   */
/* Server on the Local Device.  This function opens the Serial Port  */
/* Server on the specified RFCOMM Channel.  This function returns    */
/* zero if successful, or a negative return value if an error        */
/* occurred.                                                         */
int Bluetooth::openServer(unsigned int port){
	int  ret_val;
	char *ServiceName;

	/* First check to see if a valid Bluetooth Stack ID exists.          */
	if(bluetoothStackID)
	{
	   /* Make sure that there is not already a Serial Port Server open. */
	   if(!ServerPortID)
	   {
		 /* Simply attempt to open an Serial Server, on RFCOMM Server*/
		 /* Port 1.                                                  */
		 ret_val = SPP_Open_Server_Port(bluetoothStackID, port, SPP_Event_Callback, (unsigned long)this);

		 /* If the Open was successful, then note the Serial Port    */
		 /* Server ID.                                               */
		 if(ret_val > 0)
		 {
			/* Note the Serial Port Server ID of the opened Serial   */
			/* Port Server.                                          */
			ServerPortID = ret_val;

			/* Create a Buffer to hold the Service Name.             */
			if((ServiceName = (char*)BTPS_AllocateMemory(64)) != NULL)
			{
			   /* The Server was opened successfully, now register a */
			   /* SDP Record indicating that an Serial Port Server   */
			   /* exists. Do this by first creating a Service Name.  */
			   BTPS_SprintF(ServiceName, "Serial Port Server Port %d", port);

			   /* Now that a Service Name has been created try to    */
			   /* Register the SDP Record.                           */
			   ret_val = SPP_Register_Generic_SDP_Record(bluetoothStackID, ServerPortID, ServiceName, &SPPServerSDPHandle);

			   /* If there was an error creating the Serial Port     */
			   /* Server's SDP Service Record then go ahead an close */
			   /* down the server an flag an error.                  */
			   if(ret_val < 0)
			   {
				  SPP_Close_Server_Port(bluetoothStackID, ServerPortID);

				  /* Flag that there is no longer an Serial Port     */
				  /* Server Open.                                    */
				  ServerPortID = 0;

				  /* Flag that we are no longer connected.           */
				  Connected    = FALSE;

				  ret_val      = -1;
			   }
			   else
			   {
				  /* Simply flag to the user that everything         */
				  /* initialized correctly.                          */
				  /* Flag success to the caller.                     */
				  ret_val = 0;
			   }

			   /* Free the Service Name buffer.                      */
			   BTPS_FreeMemory(ServiceName);
			}

		 }
		 else
		 {
			ret_val = -1;
		 }

	   }
	   else
	   {
		  /* A Server is already open, this program only supports one    */
		  /* Server or Client at a time.                                 */
		 ret_val = -1;
	   }
	}
	else
	{
	   /* No valid Bluetooth Stack ID exists.                            */
	   ret_val = -1;
	}

	return(ret_val);
}

/* The following function is responsible for setting the             */
/* Discoverability Mode of the local device.  This function returns  */
/* zero on successful execution and a negative value on all errors.  */
int Bluetooth::setDiscoverabilityMode(GAP_Discoverability_Mode_t DiscoverabilityMode)
{
	return GAP_Set_Discoverability_Mode(bluetoothStackID, DiscoverabilityMode, (DiscoverabilityMode == dmLimitedDiscoverableMode)?60:0);
}

int Bluetooth::setClassOfDevice(unsigned int classOfDev)
{
   Class_of_Device_t Class_of_Device;

   /* Attempt to submit the command.                              */
   ASSIGN_CLASS_OF_DEVICE(Class_of_Device, (Byte_t)((classOfDev >> 16) & 0xFF), (Byte_t)((classOfDev >> 8) & 0xFF), (Byte_t)(classOfDev & 0xFF));

   return GAP_Set_Class_Of_Device(bluetoothStackID, Class_of_Device);
}

int Bluetooth::setConnectabilityMode(GAP_Connectability_Mode_t ConnectableMode)
{
   return GAP_Set_Connectability_Mode(bluetoothStackID, ConnectableMode);
}

int Bluetooth::setPairabilityMode(GAP_Pairability_Mode_t PairabilityMode)
{
   return GAP_Set_Pairability_Mode(bluetoothStackID, PairabilityMode);
}

void Bluetooth::init(){
	int result;
	HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, VENDOR_BAUD_RATE, cpHCILL_RTS_CTS);
	HCI_DriverInformation.DriverInformation.COMMDriverInformation.InitializationDelay = 100;

	/* Set up the application callbacks.                                 */
	BTPS_Initialization.MessageOutputCallback = displayCallback;

	/* Initialize the application.                                       */
	if((result = initializeApplication()) > 0){
		if (!setClassOfDevice(0x80510)){
			openServer(SPP_PORT_NUMBER_MINIMUM);
		}
	}
}