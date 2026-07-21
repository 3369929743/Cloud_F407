#include "main.h"
#include "OLED.h"
#include "OLED_Font.h"

#define OLED_I2C_ADDRESS       0x78U
#define OLED_COMMAND_STREAM    0x00U
#define OLED_DATA_STREAM       0x40U
#define OLED_PAGE_COUNT        8U
#define OLED_PIXEL_WIDTH       128U
#define OLED_TEXT_ROWS         4U
#define OLED_TEXT_COLUMNS      16U
#define OLED_GLYPH_WIDTH       8U
#define OLED_FIRST_CHAR        ' '
#define OLED_LAST_CHAR         '~'

static char OLED_CharCache[OLED_TEXT_ROWS][OLED_TEXT_COLUMNS];
static uint8_t OLED_CacheValid;

static inline void OLED_W_SCL(uint8_t value)
{
    #ifdef OLED_SCL_Pin
    HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, OLED_SCL_Pin,
                      value != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    #else
    (void)value;
    #endif
}

static inline void OLED_W_SDA(uint8_t value)
{
    #ifdef OLED_SDA_Pin
    HAL_GPIO_WritePin(OLED_SDA_GPIO_Port, OLED_SDA_Pin,
                      value != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    #else
    (void)value;
    #endif
}

static void OLED_I2C_Start(void)
{
    OLED_W_SDA(1U);
    OLED_W_SCL(1U);
    OLED_W_SDA(0U);
    OLED_W_SCL(0U);
}

static void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0U);
    OLED_W_SCL(1U);
    OLED_W_SDA(1U);
}

#define OLED_I2C_SEND_BIT(byte, mask)       \
    do                                       \
    {                                        \
        OLED_W_SDA(((byte) & (mask)) != 0U); \
        OLED_W_SCL(1U);                      \
        OLED_W_SCL(0U);                      \
    } while (0)

static void OLED_I2C_SendByte(uint8_t byte)
{
    OLED_I2C_SEND_BIT(byte, 0x80U);
    OLED_I2C_SEND_BIT(byte, 0x40U);
    OLED_I2C_SEND_BIT(byte, 0x20U);
    OLED_I2C_SEND_BIT(byte, 0x10U);
    OLED_I2C_SEND_BIT(byte, 0x08U);
    OLED_I2C_SEND_BIT(byte, 0x04U);
    OLED_I2C_SEND_BIT(byte, 0x02U);
    OLED_I2C_SEND_BIT(byte, 0x01U);

    /* The display is write-only here, so generate the ACK clock directly. */
    OLED_W_SDA(0U);
    OLED_W_SCL(1U);
    OLED_W_SCL(0U);
}

#undef OLED_I2C_SEND_BIT

static void OLED_I2C_BeginStream(uint8_t control)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_I2C_ADDRESS);
    OLED_I2C_SendByte(control);
}

static void OLED_I2C_EndStream(void)
{
    OLED_I2C_Stop();
}

static void OLED_WriteCommands(const uint8_t *commands, uint8_t count)
{
    OLED_I2C_BeginStream(OLED_COMMAND_STREAM);
    while (count-- != 0U)
    {
        OLED_I2C_SendByte(*commands++);
    }
    OLED_I2C_EndStream();
}

static void OLED_SetCursor(uint8_t page, uint8_t x)
{
    const uint8_t commands[] = {
        (uint8_t)(0xB0U | page),
        (uint8_t)(0x10U | (x >> 4)),
        (uint8_t)(x & 0x0FU)
    };

    OLED_WriteCommands(commands, (uint8_t)sizeof(commands));
}

static void OLED_WriteRepeatedData(uint8_t data, uint8_t count)
{
    OLED_I2C_BeginStream(OLED_DATA_STREAM);
    while (count-- != 0U)
    {
        OLED_I2C_SendByte(data);
    }
    OLED_I2C_EndStream();
}

static char OLED_NormalizeChar(char character)
{
    if ((character < OLED_FIRST_CHAR) || (character > OLED_LAST_CHAR))
    {
        return '?';
    }
    return character;
}

static void OLED_SendGlyphHalf(char character, uint8_t offset)
{
    const uint8_t *glyph = OLED_F8x16[(uint8_t)character - (uint8_t)OLED_FIRST_CHAR];

    OLED_I2C_SendByte(glyph[offset + 0U]);
    OLED_I2C_SendByte(glyph[offset + 1U]);
    OLED_I2C_SendByte(glyph[offset + 2U]);
    OLED_I2C_SendByte(glyph[offset + 3U]);
    OLED_I2C_SendByte(glyph[offset + 4U]);
    OLED_I2C_SendByte(glyph[offset + 5U]);
    OLED_I2C_SendByte(glyph[offset + 6U]);
    OLED_I2C_SendByte(glyph[offset + 7U]);
}

static void OLED_WriteGlyphRun(uint8_t line, uint8_t column,
                               const char *text, uint8_t length)
{
    uint8_t i;
    const uint8_t page = (uint8_t)(line * 2U);
    const uint8_t x = (uint8_t)(column * OLED_GLYPH_WIDTH);

    OLED_SetCursor(page, x);
    OLED_I2C_BeginStream(OLED_DATA_STREAM);
    for (i = 0U; i < length; i++)
    {
        OLED_SendGlyphHalf(text[i], 0U);
    }
    OLED_I2C_EndStream();

    OLED_SetCursor((uint8_t)(page + 1U), x);
    OLED_I2C_BeginStream(OLED_DATA_STREAM);
    for (i = 0U; i < length; i++)
    {
        OLED_SendGlyphHalf(text[i], OLED_GLYPH_WIDTH);
    }
    OLED_I2C_EndStream();
}

static void OLED_ShowText(uint8_t line, uint8_t column,
                          const char *text, uint8_t length)
{
    uint8_t i;
    uint8_t runStart;
    uint8_t runLength;

    if ((text == NULL) || (line == 0U) || (line > OLED_TEXT_ROWS) ||
        (column == 0U) || (column > OLED_TEXT_COLUMNS) || (length == 0U))
    {
        return;
    }

    line--;
    column--;
    if (length > (uint8_t)(OLED_TEXT_COLUMNS - column))
    {
        length = (uint8_t)(OLED_TEXT_COLUMNS - column);
    }

    i = 0U;
    while (i < length)
    {
        while ((i < length) && (OLED_CacheValid != 0U) &&
               (OLED_CharCache[line][column + i] == OLED_NormalizeChar(text[i])))
        {
            i++;
        }
        if (i >= length)
        {
            break;
        }

        runStart = i;
        do
        {
            OLED_CharCache[line][column + i] = OLED_NormalizeChar(text[i]);
            i++;
        } while ((i < length) &&
                 ((OLED_CacheValid == 0U) ||
                  (OLED_CharCache[line][column + i] != OLED_NormalizeChar(text[i]))));

        runLength = (uint8_t)(i - runStart);
        OLED_WriteGlyphRun(line, (uint8_t)(column + runStart),
                           &OLED_CharCache[line][column + runStart], runLength);
    }

    OLED_CacheValid = 1U;
}

static void OLED_ShowUnsignedBase(uint8_t line, uint8_t column,
                                  uint32_t number, uint8_t length,
                                  uint8_t base)
{
    char text[OLED_TEXT_COLUMNS];
    uint8_t i;

    if (length > OLED_TEXT_COLUMNS)
    {
        length = OLED_TEXT_COLUMNS;
    }

    for (i = 0U; i < length; i++)
    {
        uint8_t digit = (uint8_t)(number % base);
        text[length - i - 1U] = (char)(digit < 10U ? digit + '0' : digit - 10U + 'A');
        number /= base;
    }
    OLED_ShowText(line, column, text, length);
}

void OLED_Clear(void)
{
    uint8_t page;
    uint8_t line;
    uint8_t column;

    for (page = 0U; page < OLED_PAGE_COUNT; page++)
    {
        OLED_SetCursor(page, 0U);
        OLED_WriteRepeatedData(0x00U, OLED_PIXEL_WIDTH);
    }

    for (line = 0U; line < OLED_TEXT_ROWS; line++)
    {
        for (column = 0U; column < OLED_TEXT_COLUMNS; column++)
        {
            OLED_CharCache[line][column] = ' ';
        }
    }
    OLED_CacheValid = 1U;
}

void OLED_ShowChar(uint8_t line, uint8_t column, char character)
{
    OLED_ShowText(line, column, &character, 1U);
}

void OLED_ShowString(uint8_t line, uint8_t column, const char *string)
{
    uint8_t length = 0U;

    if (string == NULL)
    {
        return;
    }
    while ((string[length] != '\0') && (length < OLED_TEXT_COLUMNS))
    {
        length++;
    }
    OLED_ShowText(line, column, string, length);
}

void OLED_ShowNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length)
{
    OLED_ShowUnsignedBase(line, column, number, length, 10U);
}

void OLED_ShowSignedNum(uint8_t line, uint8_t column, int32_t number, uint8_t length)
{
    char text[OLED_TEXT_COLUMNS];
    uint32_t magnitude;
    uint8_t i;

    if (length >= OLED_TEXT_COLUMNS)
    {
        length = OLED_TEXT_COLUMNS - 1U;
    }

    if (number < 0)
    {
        text[0] = '-';
        magnitude = (uint32_t)(-(number + 1)) + 1U;
    }
    else
    {
        text[0] = '+';
        magnitude = (uint32_t)number;
    }

    for (i = 0U; i < length; i++)
    {
        text[length - i] = (char)((magnitude % 10U) + '0');
        magnitude /= 10U;
    }
    OLED_ShowText(line, column, text, (uint8_t)(length + 1U));
}

void OLED_ShowHexNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length)
{
    OLED_ShowUnsignedBase(line, column, number, length, 16U);
}

void OLED_ShowBinNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length)
{
    OLED_ShowUnsignedBase(line, column, number, length, 2U);
}

void OLED_Init(void)
{
    static const uint8_t initCommands[] = {
        0xAEU,
        0xD5U, 0x80U,
        0xA8U, 0x3FU,
        0xD3U, 0x00U,
        0x40U,
        0xA1U,
        0xC8U,
        0xDAU, 0x12U,
        0x81U, 0xCFU,
        0xD9U, 0xF1U,
        0xDBU, 0x30U,
        0xA4U,
        0xA6U,
        0x8DU, 0x14U,
        0xAFU
    };
    OLED_W_SCL(1U);
    OLED_W_SDA(1U);
    OLED_WriteCommands(initCommands, (uint8_t)sizeof(initCommands));
    OLED_CacheValid = 0U;
    OLED_Clear();
}
