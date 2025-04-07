struct TTNData
{
    const char LatID = '0'
    float latitude;
    const char LonID = '1'
    float longitude;
    const char AltID = '2'
    float altitude;
    const char AccID = '3'
    float hdop;
    const char SatsID = '4'
    uint8_t sats;

    uint8_t Serialize()
    {
        //return packed bytes here
        //
    }
};
