static WiFiClientSecure sslClient; // for ESP8266

const char * onSuccess = "\"Successfully invoke device method\"";
const char * notFound = "\"No method found\"";

/*
 * The new version of AzureIoTHub library change the AzureIoTHubClient signature.
 * As a temporary solution, we will test the definition of AzureIoTHubVersion, which is only defined
 *    in the new AzureIoTHub library version. Once we totally deprecate the last version, we can take
 *    the #ifdef out.
 */
#ifdef AzureIoTHubVersion
static AzureIoTHubClient iotHubClient;
void initIoThubClient()
{
    iotHubClient.begin(sslClient);
}
#else
static AzureIoTHubClient iotHubClient(sslClient);
void initIoThubClient()
{
    iotHubClient.begin();
}
#endif

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        LogInfo("Message sent to Azure IoT Hub");
        blinkLED();
    }
    else
    {
        LogInfo("Failed to send message to Azure IoT Hub");
    }
    messagePending = false;
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer)
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)buffer, strlen(buffer));
    if (messageHandle == NULL)
    {
        LogInfo("unable to create a new IoTHubMessage");
    }
    else
    {
        LogInfo("Sending message: %s", buffer);
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            LogInfo("Failed to hand over the message to IoTHubClient");
        }
        else
        {
            messagePending = true;
            LogInfo("IoTHubClient accepted the message for delivery");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

void start()
{
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
}

void stop()
{
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false; 
}

IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char* buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        LogInfo("unable to IoTHubMessage_GetByteArray");
        result = IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        /*buffer is not zero terminated*/
        LogInfo("Receive C2D message: %s", buffer);
        blinkLED();
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

int deviceMethodCallback(const char * methodName, const unsigned char * payload, size_t size, unsigned char ** response, size_t * response_size, void * userContextCallback)
{
    LogInfo("Try to invoke method %s", methodName);
    const char * responseMessage = onSuccess;
    int result = 200;
    
    if(strcmp(methodName, "start") == 0)
    {
        start();
    }
    else if(strcmp(methodName, "stop") == 0)
    {
        stop();
    }
    else
    {
        LogInfo("No method %s found", methodName);
        responseMessage = notFound;
        result = 404;
    }

    *response_size = strlen(responseMessage);
    *response = (unsigned char *)malloc(*response_size);
    strncpy((char *)(*response), responseMessage, *response_size);
    
    return result;
}