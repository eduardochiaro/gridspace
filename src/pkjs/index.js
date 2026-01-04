// GridSpace Configuration
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);


// Weather functionality
var lastWeatherData = null; // Store last weather data
var weatherEnabled = false; // Track if weather module is enabled
var weatherInterval = null; // Store interval ID for weather updates

function getLocation(successCallback, errorCallback) {
  navigator.geolocation.getCurrentPosition(
    function(position) {
      successCallback({
        lat: position.coords.latitude,
        lon: position.coords.longitude
      });
    },
    function(error) {
      errorCallback(error);
    },
    { timeout: 15000, maximumAge: 300000 } // 5 minute cache
  );
}

function fetchWeather(lat, lon, successCallback, errorCallback) {
  // Always fetch in Celsius - conversion will be done in C code
  var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon + '&current=temperature_2m&temperature_unit=celsius&timezone=auto';
  
  var xhr = new XMLHttpRequest();
  
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        try {
          var data = JSON.parse(xhr.responseText);
          if (data.current) {
            var tempCelsius = Math.round(data.current.temperature_2m);
            
            // Store weather data for caching
            lastWeatherData = {
              tempCelsius: tempCelsius
            };
            
            successCallback({
              temperature: tempCelsius, // Always send Celsius to C code
            });
          } else {
            errorCallback('Invalid weather data received');
          }
        } catch (parseError) {
          errorCallback('JSON parse error: ' + parseError.message);
        }
      } else {
        errorCallback('HTTP error! status: ' + xhr.status);
      }
    }
  };
  
  xhr.onerror = function() {
    errorCallback('Network error occurred');
  };
  
  xhr.ontimeout = function() {
    errorCallback('Request timed out');
  };
  
  xhr.timeout = 10000; // 10 second timeout
  xhr.open('GET', url, true);
  xhr.send();
}

function updateWeather() {
  console.log('Fetching weather data in Celsius...');
  
  getLocation(
    function(location) {
      console.log('Got location: ' + location.lat + ', ' + location.lon);
      
      fetchWeather(location.lat, location.lon,
        function(weather) {
          console.log('Weather: ' + weather.temperature + '°C');
          
          // Send weather data to watch (always in Celsius)
          Pebble.sendAppMessage({
            'WEATHER_TEMPERATURE': weather.temperature
          }, 
          function() {
            console.log('Weather data sent successfully');
          },
          function(error) {
            console.log('Error sending weather: ' + error);
          });
        },
        function(error) {
          console.log('Weather fetch error: ' + error);
          // Send fallback data
          Pebble.sendAppMessage({
            'WEATHER_TEMPERATURE': 0,
          });
        }
      );
    },
    function(error) {
      console.log('Location error: ' + error);
      // Send fallback data
      Pebble.sendAppMessage({
        'WEATHER_TEMPERATURE': 0,
      });
    }
  );
}

// Update weather on app start and every 30 minutes
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  
  // Read stored weather setting (default to false)
  weatherEnabled = localStorage.getItem('SHOW_WEATHER') === 'true';
  
  if (weatherEnabled) {
    console.log('Weather module enabled - starting updates');
    updateWeather();
    
    // Update weather every 30 minutes
    weatherInterval = setInterval(updateWeather, 30 * 60 * 1000);
  } else {
    console.log('Weather module disabled - skipping updates');
  }
});

// Listen for Clay configuration changes
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }
  
  var claySettings = clay.getSettings(e.response);
  console.log('Configuration received: ' + JSON.stringify(claySettings));
  
  // Check if weather module setting changed
  var newWeatherEnabled = claySettings.SHOW_WEATHER === true || claySettings.SHOW_WEATHER === 1;
  
  // Store the setting
  localStorage.setItem('SHOW_WEATHER', newWeatherEnabled.toString());
  
  // Handle weather updates based on setting
  if (newWeatherEnabled && !weatherEnabled) {
    // Weather was just enabled - start updates
    console.log('Weather module enabled - starting updates');
    weatherEnabled = true;
    updateWeather();
    if (weatherInterval) {
      clearInterval(weatherInterval);
    }
    weatherInterval = setInterval(updateWeather, 30 * 60 * 1000);
  } else if (!newWeatherEnabled && weatherEnabled) {
    // Weather was just disabled - stop updates
    console.log('Weather module disabled - stopping updates');
    weatherEnabled = false;
    if (weatherInterval) {
      clearInterval(weatherInterval);
      weatherInterval = null;
    }
  } else if (newWeatherEnabled && weatherEnabled) {
    // Weather still enabled - send cached data if available
    if (lastWeatherData) {
      console.log('Sending cached weather data to watch');
      Pebble.sendAppMessage({
        'WEATHER_TEMPERATURE': lastWeatherData.tempCelsius,
      });
    }
  }
  
  // Send all configuration to watch
  Pebble.sendAppMessage(claySettings,
    function(e) {
      console.log('Configuration sent successfully');
    },
    function(e) {
      console.log('Failed to send configuration: ' + JSON.stringify(e));
    }
  );
});