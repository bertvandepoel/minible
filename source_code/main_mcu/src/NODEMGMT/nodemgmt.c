/*!  \file     nodemgmt.c
*    \brief    Node management library
*    Created:  14/12/2018
*    Author:   Mathieu Stephan
*/
#include <assert.h>
#include <string.h>
#include "comms_hid_msgs_debug.h"
#include "nodemgmt.h"
#include "dbflash.h"
#include "main.h"

// Current node management handle
nodemgmtHandle_t nodemgmt_current_handle;
// Current date
uint16_t nodemgmt_current_date;


/*! \fn     setCurrentDate(uint16_t date)
*   \brief  Sets current date
*   \param  date    The date (see format in documentation)
*/
void setCurrentDate(uint16_t date)
{
    nodemgmt_current_date = date;
}

/*! \fn     pageNumberFromAddress(uint16_t addr)
*   \brief  Extracts a page number from a constructed address
*   \param  addr    The constructed address used for extraction
*   \return A page number in flash memory (uin16_t)
*   \note   See design notes for address format
*   \note   Max Page Number varies per flash size
 */
static inline uint16_t pageNumberFromAddress(uint16_t addr)
{
    return (addr >> NODEMGMT_ADDR_PAGE_BITSHIFT) & NODEMGMT_ADDR_PAGE_MASK_FINAL;
}

/*! \fn     nodeNumberFromAddress(uint16_t addr)
*   \brief  Extracts a node number from a constructed address
*   \param  addr   The constructed address used for extraction
*   \return A node number of a node in a page in flash memory
*   \note   See design notes for address format
*   \note   Max Node Number varies per flash size
 */
static inline uint16_t nodeNumberFromAddress(uint16_t addr)
{
    _Static_assert(NODEMGMT_ADDR_PAGE_BITSHIFT == 1, "Addressing scheme doesn't fit 1 or 2 base node size per page");
    
    #if (BYTES_PER_PAGE == BASE_NODE_SIZE)
        /* One node per page */
        return 0;
    #else
        return (addr & NODEMGMT_ADDR_NODE_MASK);
    #endif
}

/*! \fn     getIncrementedAddress(uint16_t addr)
*   \brief  Get next address for a given address
*   \param  addr   The base address
*   \return The next address for our addressing scheme
 */
uint16_t getIncrementedAddress(uint16_t addr)
{
    _Static_assert((BYTES_PER_PAGE == BASE_NODE_SIZE) || (BYTES_PER_PAGE == 2*BASE_NODE_SIZE), "Page size isn't 1 or 2 base node size");
    _Static_assert(NODEMGMT_ADDR_PAGE_BITSHIFT == 1, "Addressing scheme doesn't fit 1 or 2 base node size per page");
    
    #if (BYTES_PER_PAGE == BASE_NODE_SIZE)
        /* One node per page, change page */
        return addr + (1 << NODEMGMT_ADDR_PAGE_BITSHIFT);
    #else
        /* 2 nodes per page */
        return addr + 1;
    #endif
}

/*! \fn     nodeTypeFromFlags(uint16_t flags)
*   \brief  Gets nodeType from flags  
*   \param  flags           The flags field of a node
*   \return nodeType        See enum
*/
static inline node_type_te nodeTypeFromFlags(uint16_t flags)
{
    return (flags >> NODEMGMT_TYPE_FLAG_BITSHIFT) & NODEMGMT_TYPE_FLAG_BITMASK_FINAL;
}

/*! \fn     validBitFromFlags(uint16_t flags)
*   \brief  Gets the node valid bit from flags  
*   \return The valid 
*/
static inline uint16_t validBitFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_VALID_BIT_BITSHIFT) & NODEMGMT_VALID_BIT_MASK_FINAL);
}

/*! \fn     correctFlagsBitFromFlags(uint16_t flags)
*   \brief  Gets the correct flags valid bit from flags  
*   \return The valid 
*/
static inline uint16_t correctFlagsBitFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_CORRECT_FLAGS_BIT_BITSHIFT) & NODEMGMT_CORRECT_FLAGS_BIT_BITMASK_FINAL);
}

 /*! \fn     userIdFromFlags(uint16_t flags)
 *   \brief  Gets the user id from flags
 *   \return User ID
 */
static inline uint16_t userIdFromFlags(uint16_t flags)
{
    return ((flags >> NODEMGMT_USERID_BITSHIFT) & NODEMGMT_USERID_MASK_FINAL);
}

/*! \fn     constructDate(uint16_t year, uint16_t month, uint16_t day)
*   \brief  Packs a uint16_t type with a date code in format YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The year to pack into the uint16_t
*   \param  month           The month to pack into the uint16_t
*   \param  day             The day to pack into the uint16_t
*   \return date            The constructed / encoded date in uint16_t
*/
static inline uint16_t constructDate(uint16_t year, uint16_t month, uint16_t day)
{
    return (day | ((month << NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK) | ((year << NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK));
}

/*! \fn     extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
*   \brief  Unpacks a unint16_t to extract the year, month, and day information in format of YYYYYYYMMMMDDDDD. Year Offset from 2010
*   \param  year            The unpacked year
*   \param  month           The unpacked month
*   \param  day             The unpacked day
*   \return success status
*/
RET_TYPE extractDate(uint16_t date, uint8_t *year, uint8_t *month, uint8_t *day)
{
    *year = ((date >> NODEMGMT_YEAR_SHT) & NODEMGMT_YEAR_MASK_FINAL);
    *month = ((date >> NODEMGMT_MONTH_SHT) & NODEMGMT_MONTH_MASK_FINAL);
    *day = (date & NODEMGMT_DAY_MASK_FINAL);
    return RETURN_OK;
}

/*! \fn     checkUserPermission(uint16_t node_addr)
*   \brief  Check that the user has the right to read/write a node
*   \param  node_addr   Node address
*   \return OK / NOK
*   \note   Scanning a 8Mb Flash memory contents with that function was timed at 56ms in Debug mode.
*/
RET_TYPE checkUserPermission(uint16_t node_addr)
{
    // Future node flags
    uint16_t temp_flags;
    // Node Page
    uint16_t page_addr = pageNumberFromAddress(node_addr);
    // Node byte address
    uint16_t byte_addr = BASE_NODE_SIZE * nodeNumberFromAddress(node_addr);
    
    // Fetch the flags
    dbflash_read_data_from_flash(&dbflash_descriptor, page_addr, byte_addr, sizeof(temp_flags), (void*)&temp_flags);
    
    // Either the node belongs to us or it is invalid, check that the address is after sector 1 (upper check done at the flashread/write level)
    if ((((nodemgmt_current_handle.currentUserId == userIdFromFlags(temp_flags)) && (correctFlagsBitFromFlags(temp_flags) == NODEMGMT_VBIT_VALID)) || (validBitFromFlags(temp_flags) == NODEMGMT_VBIT_INVALID)) && (page_addr >= PAGE_PER_SECTOR))
    {
        return RETURN_OK;
    }
    else
    {
        return RETURN_NOK;
    }
}

/*! \fn     writeParentNodeDataBlockToFlash(uint16_t address, parent_node_t* parent_node)
*   \brief  Write a parent node data block to flash
*   \param  address     Where to write
*   \param  parent_node Pointer to the node
*/
void writeParentNodeDataBlockToFlash(uint16_t address, parent_node_t* parent_node)
{
    _Static_assert(BASE_NODE_SIZE == sizeof(*parent_node), "Parent node isn't the size of base node size");
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), BASE_NODE_SIZE, (void*)parent_node->node_as_bytes);
}

/*! \fn     writeChildNodeDataBlockToFlash(uint16_t address, child_node_t* child_node)
*   \brief  Write a child node data block to flash
*   \param  address     Where to write
*   \param  parent_node Pointer to the node
*/
void writeChildNodeDataBlockToFlash(uint16_t address, child_node_t* child_node)
{
    _Static_assert(2*BASE_NODE_SIZE == sizeof(*child_node), "Child node isn't twice the size of base node size");
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), BASE_NODE_SIZE, (void*)child_node->node_as_bytes);
    dbflash_write_data_to_flash(&dbflash_descriptor, pageNumberFromAddress(getIncrementedAddress(address)), BASE_NODE_SIZE * nodeNumberFromAddress(getIncrementedAddress(address)), BASE_NODE_SIZE, (void*)(&child_node->node_as_bytes[BASE_NODE_SIZE]));
}

/*! \fn     readParentNodeDataBlockFromFlash(uint16_t address, parent_node_t* parent_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void readParentNodeDataBlockFromFlash(uint16_t address, parent_node_t* parent_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), sizeof(parent_node->node_as_bytes), (void*)parent_node->node_as_bytes);
}

/*! \fn     readChildNodeDataBlockFromFlash(uint16_t address, child_node_t* child_node)
*   \brief  Read a parent node data block to flash
*   \param  address     Where to read
*   \param  parent_node Pointer to the node
*/
void readChildNodeDataBlockFromFlash(uint16_t address, child_node_t* child_node)
{
    dbflash_read_data_from_flash(&dbflash_descriptor, pageNumberFromAddress(address), BASE_NODE_SIZE * nodeNumberFromAddress(address), sizeof(child_node->node_as_bytes), (void*)child_node->node_as_bytes);
}

/*! \fn     userProfileStartingOffset(uint8_t uid, uint16_t *page, uint16_t *pageOffset)
    \brief  Obtains page and page offset for a given user id
    \param  uid             The id of the user to perform that profile page and offset calculation (0 up to NODE_MAX_UID)
    \param  page            The page containing the user profile
    \param  pageOffset      The offset of the page that indicates the start of the user profile
 */
void userProfileStartingOffset(uint16_t uid, uint16_t *page, uint16_t *pageOffset)
{
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }

    /* Check for bad surprises */    
    _Static_assert(NODEMGMT_USER_PROFILE_SIZE == sizeof(nodemgmt_userprofile_t), "User profile isn't the right size");
    
    #if BYTES_PER_PAGE == NODEMGMT_USER_PROFILE_SIZE
        /* One node per page: just return the UID */
        *page = uid;
        *pageOffset = 0;
    #elif BYTES_PER_PAGE == 2*NODEMGMT_USER_PROFILE_SIZE
        /* One node per page: bitmask and division by 2 */
        *page = uid >> NODEMGMT_ADDR_PAGE_BITSHIFT;
        *pageOffset = (uid & NODEMGMT_ADDR_PAGE_BITSHIFT) * NODEMGMT_USER_PROFILE_SIZE;
    #else
        #error "User profile isn't a multiple of page size"
    #endif
}

/*! \fn     formatUserProfileMemory(uint8_t uid)
 *  \brief  Formats the user profile flash memory of user uid.
 *  \param  uid    The id of the user to format profile memory
 */
void formatUserProfileMemory(uint16_t uid)
{
    /* Page & offset for this UID */
    uint16_t temp_page, temp_offset;
    userProfileStartingOffset(uid, &temp_page, &temp_offset);
    
    if(uid >= NB_MAX_USERS)
    {
        /* No debug... no reason it should get stuck here as the data format doesn't allow such values */
        while(1);
    }
    
    #if NODE_ADDR_NULL != 0x0000
        #error "NODE_ADDR_NULL != 0x0000"
    #endif
    
    // Set buffer to all 0's.
    memset((void*)&nodemgmt_current_handle.blob_buffer.temp_user_profile, 0, sizeof(nodemgmt_current_handle.blob_buffer.temp_user_profile));
    dbflash_write_data_to_flash(&dbflash_descriptor, temp_page, temp_offset, sizeof(nodemgmtHandle_t), (void*)&nodemgmt_current_handle.blob_buffer.temp_user_profile);
}