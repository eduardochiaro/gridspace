# GridSpace

A minimalist Pebble watchface with a distinctive pixel-grid aesthetic. Every element—from time digits to status indicators—is rendered using a uniform grid of cells, creating a clean, geometric look.

## Screenshots
### Pebble Classic/Steel
![Aplite 1](assets/aplite_1.png)
![Aplite 2](assets/aplite_2.png)

### Pebble Time/Time Steel
![Basalt animation](assets/basalt_3.gif)
![Basalt 4](assets/basalt_4.png)
![Basalt 2](assets/basalt_5.png)
![Basalt 1](assets/basalt_1.png)
![Basalt 2](assets/basalt_2.png)

### Pebble Time Round
![Chalk 1](assets/chalk_1.png)
![Chalk 2](assets/chalk_2.png)
![Chalk 3](assets/chalk_3.png)

### Pebble 2/Duo
![Diorite 1](assets/diorite_1.png)
![Diorite 2](assets/diorite_2.png)

### Pebble Time 2
![Emery 1](assets/emery_1.png)
![Emery 2](assets/emery_2.png)
![Emery 3](assets/emery_3.png)

## Store
[Rebble App Store](https://apps.rebble.io/en_US/application/694a99bbfb639a000a570b91)

## Features

### Display Elements

- **Large Time Display**: Bold 5×7 digit patterns with customizable 12/24-hour format
- **Date Display**: Compact 3×5 digits below time with fully customizable left/right components
- **Step Tracker**: 5×15 diagonal progress bar showing daily step goal (requires health service)
- **Battery Indicator**: 2×3 grid showing battery level with top-down drain visualization 

### Visual System

- **Grid-Based Rendering**: All elements use 5×5 pixel cells (6×6 on Emery)
- **Two Cell Types**: Full cells (3×3px) and partial cells (1×1px) for depth
- **Three-Color System**:
  - Background color (default: black)
  - Foreground color (default: white) - main content
  - Secondary color (default: light gray) - accents and partial cells

### Customization

Access settings through the Pebble app:

- **Color Options**: Background, foreground, and secondary colors fully customizable
- **Step Goal**: Set target steps (1,000 - 50,000, default: 8,000)
- **Display Toggles**: Show/hide steps, battery, and date
- **Time Format**: 12-hour or 24-hour display
- **Date Format**: Customize left and right sides independently with these options:
  - Month Name
  - Week Day 
  - Week of the Year
  - Day
  - Month number
  - Year
- **Load Animation**: Pick a load animation: None, Wave Fill, Random Pop, Matrix

## Platform Support

- Aplite (144×168 B&W)
- Basalt (144×168 color)
- Chalk (180×180 round color)
- Diorite (144×168 B&W)
- Emery (200×228 color, enhanced 6×6 cells)

## License

MIT License - feel free to modify and share!

