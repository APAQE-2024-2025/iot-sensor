//on god frfr no cap no yap js is goofy ahh

const LITTLE_ENDIAN = true;

const MinGPSSats = 4;
const XOffset = 2 - MinGPSSats;

const batteryVoltagePoints = [
    { x: 2.5, y: 0 },
    { x: 3.0, y: 10 },
    { x: 3.2, y: 20 },
    { x: 3.4, y: 30 },
    { x: 3.5, y: 40 },
    { x: 3.6, y: 50 },
    { x: 3.7, y: 60 },
    { x: 3.8, y: 70 },
    { x: 3.9, y: 80 },
    { x: 4.0, y: 90 },
    { x: 4.2, y: 100 }
  ];

let cursor = 0;

// struct PayloadData
// {
//     float longitude;
//     float latitude;
//     float altitude;
//     float hdop;
//     uint8_t satellites;
//     uint64_t PeePeePooPooTest = 69;
// };

const DataTypes =
{
    Float:  { size: 4, id: 0 },
    Double: { size: 8, id: 1 },
    UInt32: { size: 4, id: 2 },
    UInt64: { size: 8, id: 3 },
    Int32:  { size: 4, id: 4 },
    Int64:  { size: 8, id: 5 },
    Uint8:  { size: 1, id: 6 }
};

function Decoder(bytes, port) 
{
    switch(port) {
        case 10:
          break;
        case 69:
            let error = String.fromCharCode.apply(null, bytes);
            return { error:("ERROR: " + error) };
        default:
            throw new Error("Unknown port");
      }
    
    let bufferLen = bytes.length;
    const buffer = new ArrayBuffer(bufferLen);
    const view = new DataView(buffer);

    for (let i = 0; i < bufferLen; i++) 
    {
        view.setUint8(i, bytes[i]);
    }

    let voltage;
    let gasResistance;
    let humidity;
    let temperature;
    let pressure;
    let voc;
    let flags = read(DataTypes.Uint8, view);
    cursor += 3; //shit... forgot to turn on struct packing...

    decode={ 
        port: port,
        wisdom: "cheese",
        measurements :[
            {
                name: "Pressure",
                value: (pressure = read(DataTypes.Float, view) * 10), //bro i swear to god i fixed this on the µcontroller
                description: "[hPa] Air pressure"
            },
            {
                name: "RawHumidity",
                value: (humidity = read(DataTypes.Float, view)),
                description: "[%] Temperature uncompensated relative humidity"
            },
            {
                name: "GasResistance",
                value: (gasResistance = read(DataTypes.Float, view)),
                description: "[Ω] Electrical resistance of the air, lower indicates more pollutants (VOC)"
            },
            {
                name: "StabStatus",
                value: read(DataTypes.Float, view),
                description: "[0/1] Stabilization status of the heating element of the sensor (whether measurements can be trusted)"
            },
            {
                name: "RunInStatus",
                value: read(DataTypes.Float, view),
                description: "[0/1] Calibration status, due to Bosche's incompetance this will (probably) always be 0"
            },
            {
                name: "RawTemperature",
                value: (temperature = read(DataTypes.Float, view)),
                description: "[°C] Temperature which has not been compensated for the heating element in the sensor"
            },
            {
                name: "BatteryVoltage",
                value: (voltage = read(DataTypes.Float, view)),
                description: "[V] Current measured voltage of the Li-ion cell in the device"
            },
            {
                name: "BatteryPercentage",
                value: getBatteryPercentage(voltage),
                description: "[%] An estimated battery percentage"
            }
        ]
    };

    if((flags & 1) !== 0)
    {
        decode.measurements = 
        [
            ...decode.measurements,
            {
                name: "IAQ",
                value: read(DataTypes.Float, view),
                description: "[0-500] Indoor airquality index"
            },
            {
                name: "IAQAccuracy",
                value: read(DataTypes.Float, view),
                description: "[0-3] a measure of how accurate the IAQ is where 0 is basically garbage and 3 is great"
            },
            {
                name: "StaticIAQ",
                value: read(DataTypes.Float, view),
                description: "[0-500] Unscaled IAQ"
            }, 
            {
                name: "RelativeCo2Equivalent",
                value: read(DataTypes.Float, view),
                description: "[ppm] A measure of CO2 relative to the measured baseline of the sensor's calibration data (usually +/-600 for average CO2)"
            }, 
            {
                name: "RelativeVocEquivalent",
                value: read(DataTypes.Float, view),
                description: "[??] A measure of VOC relative to the measured baseline of the sensor's calibration data"
            },
            {
                name: "GasPercentage",
                value: read(DataTypes.Float, view) * 100,
                description: "[%] The percentage of the air that is polluting gasses"
            },
            {
                name: "Temperature",
                value: (temperature = read(DataTypes.Float, view)),
                description: "[°C] Temperature compensated with the sensor's heating element"
            },
            {
                name: "Humidity",
                value: (humidity = read(DataTypes.Float, view)),
                description: "[%] Temperature Compensated relative humidity"
            }
        ];
    }

    decode.measurements = 
    [
        ...decode.measurements,
        {
            name: "WeatherVibes",
            value: getWeatherVibes(pressure, humidity, temperature),
            description: "[0-5] A measure of how comfortable the weather is where 0 is probable rain/colder/harsher and 5 is nice humidity/temperature and low chance of rain"
        },
        {
            name: "AirQualityScore",
            value: calculateAirQualityScore(voc, humidity, temperature),
            description: "[0-1] A measure of how clean the air is with 0 being bad and 1 being good"
        },
        {
            name: "AbsoluteHumidity",
            value: getAbsoluteHumidity(temperature, humidity, pressure),
            description: "[g/m³] Absolute humidity (the actual quantity of water in the air, not relative)"
        },
        {
            name: "VOC",
            value: (voc = guestimateVoc(gasResistance, humidity, temperature)),
            description: "[ppb] Estimated absolute VOC levels"            
        }
    ];

    return decode;
}

function read(dataType, view)
{
    let result;
    switch (dataType.id) 
    {
        case 0:
            result = view.getFloat32(cursor, LITTLE_ENDIAN);
            break;
        case 1:
            result = view.getFloat64(cursor, LITTLE_ENDIAN);
            break;
        case 2:
            result = view.getUint32(cursor, LITTLE_ENDIAN);
            break;
        case 3:
            result = view.getBigUint64(cursor, LITTLE_ENDIAN);
            break;
        case 4:
            result = view.getInt32(cursor, LITTLE_ENDIAN);
            break;
        case 5:
            result = view.getBigInt64(cursor, LITTLE_ENDIAN);
            break;
        case 6:
            result = view.getUint8(cursor, LITTLE_ENDIAN);
            break;
        default:
            throw new Error("UNKNOWN TYPE");
    }

    cursor += dataType.size;
    return result;
}

//thx gyattGPT for translating this to js from a propper language (C#) :)
//this shit is just some quadratic interpolation between some points i googled
//like linear interpolation... just less linear...
//it works well... enough
function getBatteryPercentage(voltage)
{
    const nearestPoints = batteryVoltagePoints
      .slice()
      .sort((a, b) => Math.abs(a.x - voltage) - Math.abs(b.x - voltage))
      .slice(0, 3)
      .sort((a, b) => a.x - b.x);
  
    const [p0, p1, p2] = nearestPoints;
    const x0 = p0.x, y0 = p0.y;
    const x1 = p1.x, y1 = p1.y;
    const x2 = p2.x, y2 = p2.y;
  
    // Lagrange interpolation
    const L0 = ((voltage - x1) * (voltage - x2)) / ((x0 - x1) * (x0 - x2));
    const L1 = ((voltage - x0) * (voltage - x2)) / ((x1 - x0) * (x1 - x2));
    const L2 = ((voltage - x0) * (voltage - x1)) / ((x2 - x0) * (x2 - x1));
  
    const percentage = y0 * L0 + y1 * L1 + y2 * L2;
  
    return percentage;
}

//vibe coded by chatGPT based on the BME680 datasheet, hopes/dreams and vibes
function guestimateVoc(gas_resistance_ohms, humidity_rh, temperature_c) 
{
    // --- Baseline values (tune these for your location/environment) ---
    const gas_baseline = 100000;      // 100kΩ = very clean air
    const hum_baseline = 40.0;        // 40% RH is Bosch's "neutral" point
    const temp_baseline = 25.0;       // 25°C room temp

    // --- Normalize readings ---
    let gas_ratio = gas_baseline / gas_resistance_ohms;
    if (gas_ratio < 1) gas_ratio = 1;  // avoid nonsense low pollution
    let voc_log = Math.log10(gas_ratio);

    // --- Humidity influence ---
    let humidity_offset = humidity_rh - hum_baseline;
    let humidity_factor = 1 + 0.02 * humidity_offset;

    // --- Temperature influence ---
    let temp_offset = temperature_c - temp_baseline;
    let temp_factor = 1 + 0.01 * temp_offset;

    // --- Combine influences ---
    let voc_index = voc_log * 400 * humidity_factor * temp_factor;

    // --- Clamp and return ---
    if (voc_index < 0) voc_index = 0;
    if (voc_index > 1000) voc_index = 1000;

    return Math.round(voc_index); // estimated ppb
}

//vaguely based on "Indoor Air Quality Index" but also mostly on vibes
function calculateAirQualityScore(VOC_ppb, humidity, temperature) 
{
    // VOC Score: Assuming 0-500 ppb range
    let vocScore = 1 - Math.min(VOC_ppb / 500, 1);

    // Humidity Score: Optimal at 40-60%
    let humidityScore = 1 - Math.min(Math.abs(humidity - 50) / 50, 1);

    // Temperature Score: Optimal at 20-26°C
    let tempScore = 1 - Math.min(Math.abs(temperature - 23) / 23, 1);

    // Weighted average
    let airQualityScore = (vocScore * 0.5) + (humidityScore * 0.25) + (tempScore * 0.25);

    return Math.round(airQualityScore * 1000) / 1000; // Rounded to 3 decimal places
}

//again...vaguely based on "Temperature-humidity index" but also mostly on vibes
function getWeatherVibes(pressure, humidity, temperature) 
{
    // Pressure Score: Optimal at 1013 hPa
    let pressureScore = 1 - Math.min(Math.abs(pressure - 1013) / 20, 1);

    // Humidity Score: Optimal at 40-60%
    let humidityScore = 1 - Math.min(Math.abs(humidity - 50) / 50, 1);

    // Temperature Score: Optimal at 22-28°C
    let tempScore = 1 - Math.min(Math.abs(temperature - 25) / 25, 1);

    // Weighted average
    let comfortScore = (pressureScore * 0.3) + (humidityScore * 0.3) + (tempScore * 0.4);

    // Scale to 0-5
    return Math.round(comfortScore * 5);
}

//based on the Magnus formula, ideal gas law and https://en.wikipedia.org/wiki/Absolute_humidity
function getAbsoluteHumidity(tempC, relHumidity, pressure_hPa = 1013.25) 
{
    const R = 8.314462618;    // J/(mol·K)
    const Mw = 18.01534;      // g/mol
    const T = tempC + 273.15; // K

    // Tetens/Magnus formula to estimate saturation vapor pressure (hPa)
    const A = 6.112;
    const m = 17.62;
    const Tn = 243.12;
    const Pws = A * Math.exp((m * tempC) / (tempC + Tn)); // hPa

    // Actual vapor pressure (hPa)
    const Pw = relHumidity / 100 * Pws;

    // Convert hPa to Pa
    const Pw_Pa = Pw * 100;

    // Absolute humidity in g/m³ using ideal gas law
    const absHumidity = (Pw_Pa * Mw) / (R * T);
    return absHumidity;
}