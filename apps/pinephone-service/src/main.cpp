#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

#include <sx126x_driver/SX126x.hpp>
#include <PineDio/LoRa/PinephoneBackplate.h>
#include <PineDio/LoRa/Exceptions.h>
using namespace PineDio::LoRa;

std::atomic<int*> target_fd_pointer;
std::atomic<PineDio::LoRa::PinephoneBackplate*> radio_ptr;

void* listen(void* param){
    PineDio::LoRa::PinephoneBackplate* radio = radio_ptr.load();
    while(true){
        // TODO: does this block until interrupt has happened? or do we need to sleep a bit in this loop to reduce CPU uses?
        // note: this will trigger data_received when data has been recevied by the radio
        radio->ProcessIrqs();
    }
}

void data_received(){
    // get data from the buffer
    uint8_t size = 0;
    uint8_t offset = 0;
    PineDio::LoRa::PinephoneBackplate* radio = radio_ptr.load();
    radio->GetRxBufferStatus(&size, &offset);
    std::vector<uint8_t> buffer;
    buffer.resize(size);
    radio->GetPayload(buffer.data(), &size, size);

    // TODO: remove
    std::cout << "Received lora data: " << buffer.data() << std::endl;

    // pass data to the unix socket
    int* fd = target_fd_pointer.load();
    if(fd){
        // write the size
        uint16_t packetsize = buffer.size();
        int ret = write(*fd, (char*)&packetsize, sizeof(packetsize));
        if(ret == -1){
            std::cerr << "Error: failed to write to socket: " << errno << std::endl;
        }
        // and the buffer itself
        ret = write(*fd, buffer.data(), packetsize);
        if(ret == -1){
            std::cerr << "Error: failed to write to socket: " << errno << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc < 3){
        std::cerr << "Missing arguments. Usage: pinephone-service [socket path] [i2c device path]" << std::endl;
        return -1;
    }

    // startup socket server
    int sock_fd = 0;
    if((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        std::cerr << "Error: failed to create socket" << std::endl;
        return 1;
    }
    
    // get the socket path
    std::string socket_path(argv[1]);

    // remove existing socket
    unlink(socket_path.c_str());

    // setup socket address
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_path.c_str());

    // bind to the socket address
    if(bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1){
        std::cerr << "Error: failed to bind socket" << std::endl;
        return 2;
    }

    // listen for incoming connections
    if(listen(sock_fd, 1) == -1){
        std::cerr << "Error: failed to listen to socket" << std::endl;
        return 3;
    }

    // setup lora driver
    PineDio::LoRa::PinephoneBackplate radio(argv[2]);
    radio_ptr.store(&radio);
    radio.Initialize();

    // setup handlers
    radio.callbacks.rxDone = [](){data_received();};
    radio.callbacks.txDone = [](){
        PineDio::LoRa::PinephoneBackplate* radio = radio_ptr.load();
        radio->SetRx(0xffffffff);
    };

    // radio config
    radio.SetDeviceType(SX126x::SX1262);
    radio.Init();
    radio.SetDio2AsRfSwitchCtrl(true);
    radio.SetStandby(SX126x::RadioStandbyModes_t::STDBY_RC);
    radio.SetRegulatorMode(SX126x::RadioRegulatorMode_t::USE_DCDC);
    radio.SetBufferBaseAddresses(0,127);

    radio.SetTxParams(22, SX126x::RadioRampTimes_t::RADIO_RAMP_3400_US);
    radio.SetDioIrqParams(0xffff, 0x0001, 0x0000, 0x0000);
    radio.SetRfFrequency(868000000); // Europe band. US: 915 MHz, Asia: 169 MHz, 433 MHz

    radio.SetPacketType(SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA);

    radio.SetStopRxTimerOnPreambleDetect(false);

    SX126x::ModulationParams_t modulationParams;
    modulationParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;
    modulationParams.Params.LoRa.LowDatarateOptimize = 0;
    modulationParams.Params.LoRa.Bandwidth = SX126x::RadioLoRaBandwidths_t::LORA_BW_500; // has to be 250 on EU?
    modulationParams.Params.LoRa.CodingRate = SX126x::RadioLoRaCodingRates_t::LORA_CR_4_5;
    modulationParams.Params.LoRa.SpreadingFactor = SX126x::RadioLoRaSpreadingFactors_t::LORA_SF12; // set this closer to 7 to increase the bitrate
    radio.SetModulationParams(modulationParams);

    // setup message TODO: can we remove this?
    static const char* message = "Hello, I'm a Pinephone!";
    auto s = strlen(message);
    SX126x::PacketParams_t packetParams;
    packetParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;;
    packetParams.Params.LoRa.HeaderType = SX126x::RadioLoRaPacketLengthsMode_t::LORA_PACKET_VARIABLE_LENGTH;
    packetParams.Params.LoRa.InvertIQ = SX126x::RadioLoRaIQModes_t::LORA_IQ_INVERTED;
    packetParams.Params.LoRa.CrcMode = SX126x::RadioLoRaCrcModes_t::LORA_CRC_ON;
    packetParams.Params.LoRa.PayloadLength = s;
    packetParams.Params.LoRa.PreambleLength = 8;
    radio.SetPacketParams(packetParams);

    // clear status
    radio.ClearIrqStatus(0xffff);
    radio.SetRx(0xffffffff);

    // spin up thread for listening
    pthread_t listening_thread;
    int ret = pthread_create(&listening_thread, NULL, &(listen), NULL);
    if(ret != 0){
        std::cerr << "Error: failed to create thread: " << ret << std::endl;
    }

    // accept connections (iteratively as we only have a single client)
    struct sockaddr_un client_addr;
    int cur_sock_fd = 0;
    while(true){
        // accept connection
        socklen_t client_len = sizeof(client_addr);
        if((cur_sock_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_len)) == -1){
            std::cerr << "Error: failed to accept socket" << std::endl;
            continue;
        }
        target_fd_pointer.store(&cur_sock_fd);
        
        std::cout << "Connection accepted" << std::endl;

        // do reads from the socket
        while(true){
            // read 2 bytes into val
            uint16_t item_length = 0;
            int ret = read(cur_sock_fd, &item_length, 2);

            std::vector<uint8_t> buffer;
            buffer.resize(item_length);
            ret = read(cur_sock_fd, &buffer[0], item_length);
            if(ret == -1){
                std::cerr << "Error: failed to read socket" << std::endl;
                break;
            }
            // if read returns empty, we know the connection has closed
            if(ret == 0){
                std::cout << "Read empty data, connection closed" << std::endl;
                break;
            }

            // send the buffer over the radio
            SX126x::PacketParams_t packetParams;
            packetParams.PacketType = SX126x::RadioPacketTypes_t::PACKET_TYPE_LORA;;
            packetParams.Params.LoRa.HeaderType = SX126x::RadioLoRaPacketLengthsMode_t::LORA_PACKET_VARIABLE_LENGTH;
            packetParams.Params.LoRa.InvertIQ = SX126x::RadioLoRaIQModes_t::LORA_IQ_INVERTED;
            packetParams.Params.LoRa.CrcMode = SX126x::RadioLoRaCrcModes_t::LORA_CRC_ON;
            packetParams.Params.LoRa.PayloadLength = item_length;
            packetParams.Params.LoRa.PreambleLength = 8;
            radio.SetPacketParams(packetParams);
            radio.SendPayload(buffer.data(), item_length, 0xffffffff);

            std::cout << "Sending buffer: " << buffer.data() << std::endl; // TODO: remove
        }

        // close connection and clear the atomic pointer
        std::cout << "Close connection" << std::endl;
        target_fd_pointer.store(NULL);
        close(cur_sock_fd);
    }

    return 0;
}