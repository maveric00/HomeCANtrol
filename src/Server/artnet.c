/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * artnet.c
 * Implementes the external functions for libartnet
 * Copyright (C) 2004-2007 Simon Newton
 */
#include "private.h"

// various constants used everywhere
int ARTNET_ADDRESS_NO_CHANGE = 0x7f;
int ARTNET_PORT = 6454;
int ARTNET_STRING_SIZE = 8;
char ARTNET_STRING[] = "Art-Net";
uint8_t ARTNET_VERSION = 14;
uint8_t OEM_HI = 0x04;
uint8_t OEM_LO = 0x30;
char ESTA_HI = 'z';
char ESTA_LO = 'p';
uint8_t TTM_BEHAVIOUR_MASK = 0x02;
uint8_t TTM_REPLY_MASK = 0x01;
uint8_t PROGRAM_NO_CHANGE = 0x7f;
uint8_t PROGRAM_DEFAULTS = 0x00;
uint8_t PROGRAM_CHANGE_MASK = 0x80;
uint8_t HIGH_NIBBLE = 0xF0;
uint8_t LOW_NIBBLE = 0x0F;
uint8_t STATUS_PROG_AUTH_MASK = 0x30;
uint8_t PORT_STATUS_LPT_MODE = 0x02;
uint8_t PORT_STATUS_SHORT = 0x04;
uint8_t PORT_STATUS_ERROR = 0x04;
uint8_t PORT_STATUS_DISABLED_MASK = 0x08;
uint8_t PORT_STATUS_MERGE = 0x08;
uint8_t PORT_STATUS_DMX_TEXT = 0x10;
uint8_t PORT_STATUS_DMX_SIP = 0x20;
uint8_t PORT_STATUS_DMX_TEST = 0x40;
uint8_t PORT_STATUS_ACT_MASK = 0x80;
uint8_t PORT_DISABLE_MASK = 0x01;
uint8_t MIN_PACKET_SIZE = 10;
uint8_t MERGE_TIMEOUT_SECONDS = 10;
uint8_t RECV_NO_DATA = 1;
uint8_t MAX_NODE_BCAST_LIMIT = 30; // always bcast after this point

#ifndef TRUE
int TRUE = 1;
int FALSE = 0;
#endif

uint16_t LOW_BYTE = 0x00FF;
uint16_t HIGH_BYTE = 0xFF00;

enum { INITIAL_IFACE_COUNT = 10 };
enum { IFACE_COUNT_INC = 5 };
enum { IFNAME_SIZE = 32 }; // 32 sounds a reasonable size

typedef struct iface_s {
  struct sockaddr_in ip_addr;
  struct sockaddr_in bcast_addr;
  int8_t hw_addr[ARTNET_MAC_SIZE];
  char   if_name[IFNAME_SIZE];
  struct iface_s *next;
} iface_t;

unsigned long LOOPBACK_IP = 0x7F000001;

void copy_apr_to_node_entry(artnet_node_entry e, artnet_reply_t *reply);
int find_nodes_from_uni(node_list_t *nl, uint8_t uni, SI *ips, int size);

uint8_t _make_addr(uint8_t subnet, uint8_t addr);
void check_merge_timeouts(node n, int port);
void merge(node n, int port, int length, uint8_t *latest);


// static buffer for the error strings
char artnet_errstr[256];

/*
 * Libartnet error function
 * This writes the error string to artnet_errstr, which can be accessed
 * using artnet_strerror();
 */
void artnet_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(artnet_errstr, sizeof(artnet_errstr), fmt, ap);
  va_end(ap);
}


/*
 * Converts 4 bytes in big endian order to a 32 bit int
 */
int32_t artnet_misc_nbytes_to_32(uint8_t bytes[4]) {
  return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

/*
 * Converts an int to an arrany of 4 bytes in big endian format
 */
void artnet_misc_int_to_bytes(int data, uint8_t *bytes) {
    bytes[3] = (data & 0x000000FF);
    bytes[2] = (data & 0x0000FF00) >> 8;
    bytes[1] = (data & 0x00FF0000) >> 16;
    bytes[0] = (data & 0xFF000000) >> 24;
}

/*
 * Creates a new ArtNet node.
 * Takes a string containing the ip address to bind to, if the string is NULL
 * it uses the first non loopback address
 *
 * @param ip the IP address to bind to
 * @param debug level of logging provided 0: none
 * @return an artnet_node, or NULL on failure
 */
artnet_node artnet_new(const char *ip, int verbose) {
  node n;
  int i;

  n = malloc(sizeof(artnet_node_t));

  if (!n) {
    artnet_error("malloc failure");
    return NULL;
  }

  memset(n, 0x0, sizeof(artnet_node_t));

  // init node listing
  n->node_list.first = NULL;
  n->node_list.current = NULL;
  n->node_list.last = NULL;
  n->node_list.length = 0;
  n->state.verbose = verbose;
  n->state.oem_hi = OEM_HI;
  n->state.oem_lo = OEM_LO;
  n->state.esta_hi = ESTA_HI;
  n->state.esta_lo = ESTA_LO;
  n->state.bcast_limit = 0;

  n->peering.peer = NULL;
  n->peering.master = TRUE;

  n->sd = INVALID_SOCKET;

  if (artnet_net_init(n, ip)) {
    free(n);
    return NULL;
  }

  // now setup the default parameters
  n->state.send_apr_on_change = FALSE;
  n->state.ar_count = 0;
  n->state.report_code = ARTNET_RCPOWEROK;
  n->state.reply_addr.s_addr = 0;
  n->state.mode = ARTNET_STANDBY;

  // set all ports to MERGE HTP mode and disable
  for (i=0; i < ARTNET_MAX_PORTS; i++) {
    n->ports.out[i].merge_mode = ARTNET_MERGE_HTP;
    n->ports.out[i].port_enabled = FALSE;
    n->ports.in[i].port_enabled = FALSE;
  }
  return n;
}


/*
 * Starts the ArtNet node.
 * Binds the network socket and sends an ArtPoll
 * @param vn the artnet_node
 * @return 0 on success, non 0 on failure
 *
 */
int artnet_start(artnet_node vn) {
  node n = (node) vn;
  int ret;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY)
    return ARTNET_ESTATE;

  if ((ret = artnet_net_start(n)))
    return ret;

  n->state.mode = ARTNET_ON;

  if (n->state.reply_addr.s_addr == 0) {
    n->state.reply_addr = n->state.bcast_addr;
  }

  // build the initial reply
  if ((ret = artnet_tx_build_art_poll_reply(n)))
    return ret;

  if (n->state.node_type == ARTNET_SRV) {
    // poll the network
    if ((ret = artnet_tx_poll(n,NULL, ARTNET_TTM_AUTO)))
      return ret;
  } else {
    // send a reply on startup
    if ((ret = artnet_tx_poll_reply(n, FALSE)))
      return ret;
  }
  return ret;
}


/*
 * Stops the ArtNet node. This closes the network sockets held by the node
 * @param vn the artnet_node
 * @return 0 on success, non-0 on failure
 */
int artnet_stop(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  artnet_net_close(n->sd);
  n->state.mode = ARTNET_STANDBY;
  return ARTNET_EOK;
}


/*
 * Free the memory associated with this node
 */
int artnet_destroy(artnet_node vn) {
  node n = (node) vn;
  node_entry_private_t *ent, *tmp;
  int i;

  check_nullnode(vn);

  // free any memory associated with firmware transfers
  for (ent = n->node_list.first; ent != NULL; ent = tmp) {
    tmp = ent->next;
    free(ent);
  }

  free(vn);
  return ARTNET_EOK;
}


/*
 * Set the OEM code
 * This can only be done in the standby state
 */
int artnet_setoem(artnet_node vn, uint8_t hi, uint8_t lo) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY)
    return ARTNET_ESTATE;

  n->state.oem_hi = hi;
  n->state.oem_lo = lo;
  return ARTNET_EOK;
}


/*
 * Set the ESTA code
 * This can only be done in the standby state
 */
int artnet_setesta(artnet_node vn, char hi, char lo) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_STANDBY)
    return ARTNET_ESTATE;

  n->state.esta_hi = hi;
  n->state.esta_lo = lo;
  return ARTNET_EOK;
}


/*
 * Set the number of nodes above which we start to bcast data
 * @param vn the artnet_node
 * @param limit 0 to always broadcast
 */
int artnet_set_bcast_limit(artnet_node vn, int limit) {
  node n = (node) vn;
  check_nullnode(vn);

  if (limit > MAX_NODE_BCAST_LIMIT) {
    artnet_error("attempt to set bcast limit > %d", MAX_NODE_BCAST_LIMIT);
    return ARTNET_EARG;
  }

  n->state.bcast_limit = limit;
  return ARTNET_EOK;
}

/*
 * Handle any received packets.
 * This function is the workhorse of libartnet. You have a couple of options:
 *   - use artnet_get_sd() to retrieve the socket descriptors and select to
 *     detect network activity. Then call artnet_read(node,0)
 *     when activity is detected.
 *   - call artnet_read repeatedly from within a loop with an appropriate
 *     timeout
 *
 * @param vn the artnet_node
 * @param timeout the number of seconds to block for if nothing is pending
 * @return 0 on success, -1 on failure
 */
int artnet_read(artnet_node vn, int timeout) {
  node n = (node) vn;
  node tmp;
  artnet_packet_t p;
  int ret;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  while (1) {
    memset(&p.data, 0x0, sizeof(p.data));

    // check timeouts now, else this packet may update the timestamps

    if ((ret = artnet_net_recv(n, &p, timeout)) < 0)
      return ret;

    // nothing to read
    if (ret == RECV_NO_DATA)
      break;

    // skip this packet (filtered)
    if (p.length == 0)
      continue;


    if (p.length > MIN_PACKET_SIZE && get_type(&p)) {
      handle(n, &p);
      for (tmp = n->peering.peer; tmp != NULL && tmp != n; tmp = tmp->peering.peer) {
        handle(tmp, &p);
      }
    }
  }
  return ARTNET_EOK;
}


/*
 * To get around the 4 universes per node limitation , we can start more than
 * one node on different ip addresses - you'll need to add aliases to your
 * network interface something like:
 *
 * $ ifconfig eth0:1 10.0.0.10 netmask 255.255.255.0
 *
 * Then the nodes must be joined so that they can share the socket
 * bound to the broadcast address.
 * TODO: use IP_PKTINFO so that packets are sent from the correct source ip
 *
 * @param vn1 The artnet node
 * @param vn2 The second artnet node
 *
 * @return 0 on sucess, non 0 on failure
 */
int artnet_join(artnet_node vn1, artnet_node vn2) {

  check_nullnode(vn1);
  check_nullnode(vn2);

  node n1 = (node) vn1;
  node n2 = (node) vn2;
  node tmp, n;

  if (n1->state.mode == ARTNET_ON || n2->state.mode == ARTNET_ON) {
    artnet_error("%s called after artnet_start", __FUNCTION__);
    return ARTNET_EACTION;
  }

  tmp = n1->peering.peer == NULL ? n1 : n1->peering.peer;
  n1->peering.peer = n2;
  for (n = n2; n->peering.peer != NULL && n->peering.peer != n2; n = n->peering.peer) ;
  n->peering.peer = tmp;

  // make sure there is only 1 master
  for (n = n1->peering.peer; n != n1; n = n->peering.peer)
    n->peering.master = FALSE;

  n1->peering.master = TRUE;

  return ARTNET_ESTATE;
}


/*
 * This is used to set handlers for sent/received artnet packets.
 * If you're using a stock standard node you more than likely don't want
 * to set these. See the artnet_set_dmx_callback and artnet_set_firmware_callback.
 * If you want to get down and dirty with artnet packets, you can set this
 * read / manipulate packets as they arrive (or get sent)
 *
 * @param vn The artnet_node
 * @param handler The handler to set
 * @param fh A pointer to a function, set to NULL to turn off
 *           The function should return 0,
 * @param data Data to be passed to the handler when its called
 * @return 0 on sucess, non 0 on failure
 */
int artnet_set_handler(artnet_node vn,
                       artnet_handler_name_t handler,
                       int (*fh)(artnet_node n, void *pp, void * d),
                       void *data) {
  node n = (node) vn;
  callback_t *callback;
  check_nullnode(vn);

  switch(handler) {
    case ARTNET_RECV_HANDLER:
      callback = &n->callbacks.recv;
      break;
    case ARTNET_SEND_HANDLER:
      callback = &n->callbacks.send;
      break;
    case ARTNET_POLL_HANDLER:
      callback = &n->callbacks.poll;
      break;
    case ARTNET_REPLY_HANDLER:
      callback = &n->callbacks.reply;
      break;
    case ARTNET_ADDRESS_HANDLER:
      callback = &n->callbacks.address;
      break;
    case ARTNET_INPUT_HANDLER:
      callback = &n->callbacks.input;
      break;
    case ARTNET_DMX_HANDLER:
      callback = &n->callbacks.dmx;
      break;
    default:
      artnet_error("%s : Invalid handler defined", __FUNCTION__);
      return ARTNET_EARG;
  }
  callback->fh = fh;
  callback->data = data;
  return ARTNET_EOK;
}


/*
 * This is a special callback which is invoked when dmx data is received.
 *
 * @param vn The artnet_node
 * @param fh The callback to invoke (parameters passwd are the artnet_node, the port_id
 *           that received the dmx, and some user data
 * @param data    Data to be passed to the handler when its called
 */
int artnet_set_dmx_handler(artnet_node vn,
                           int (*fh)(artnet_node n, int port, void *d),
                           void *data) {
  node n = (node) vn;
  check_nullnode(vn);

  n->callbacks.dmx_c.fh = fh;
  n->callbacks.dmx_c.data = data;
  return ARTNET_EOK;
}



/*
 * @param vn     The artnet_node
 * @param fh    The callback to invoke (parameters passwd are the artnet_node, a value which
 *           is true if this was a ubea upload, and some user data
 * @param data    Data to be passed to the handler when its called
 */
int artnet_set_program_handler(artnet_node vn,
                               int (*fh)(artnet_node n, void *d),
                               void *data) {
  node n = (node) vn;
  check_nullnode(vn);
  n->callbacks.program_c.fh = fh;
  n->callbacks.program_c.data = data;
  return ARTNET_EOK;
}

// sends a poll to the specified ip, or if null, will broadcast
// talk_to_me - modify remote nodes behaviour, see spec
// TODO - this should clear the node list - but this will cause issues if the caller holds references
//   to certain nodes

/**
 *
 * @param vn the artnet_node
 * @param ip the ip address to send to, NULL will broadcast the ArtPoll
 * @param talk_to_me the value for the talk to me
 */
int artnet_send_poll(artnet_node vn,
                     const char *ip,
                     artnet_ttm_value_t talk_to_me) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    return artnet_tx_poll(n, ip, talk_to_me);
  }

  artnet_error("%s : Not sending poll, not a server or raw device", __FUNCTION__);
  return ARTNET_ESTATE;
}


/*
 * Sends an artpoll reply
 *
 * @param vn the artnet_node
 */
int artnet_send_poll_reply(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  return artnet_tx_poll_reply(n, FALSE);
}


/*
 * Sends some dmx data
 *
 * @param vn the artnet_node
 */
int artnet_send_dmx(artnet_node vn,
                    int port_id,
                    int16_t length,
                    const uint8_t *data) {
  node n = (node) vn;
  artnet_packet_t p;
  int ret;
  input_port_t *port;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }
  port = &n->ports.in[port_id];

  if (length < 1 || length > ARTNET_DMX_LENGTH) {
    artnet_error("%s : Length of dmx data out of bounds (%i < 1 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
    return ARTNET_EARG;
  }

  if (port->port_status & PORT_STATUS_DISABLED_MASK) {
    artnet_error("%s : attempt to send on a disabled port (id:%i)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }

  // ok we're going to send now, make sure we turn the activity bit on
  port->port_status = port->port_status | PORT_STATUS_ACT_MASK;

  p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

  // now build packet
  memcpy(&p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.admx.opCode =  htols(ARTNET_DMX);
  p.data.admx.verH = 0;
  p.data.admx.ver = ARTNET_VERSION;
  p.data.admx.sequence = port->seq;
  p.data.admx.physical = port_id;
  p.data.admx.universe = htols(port->port_addr);

  // set length
  p.data.admx.lengthHi = short_get_high_byte(length);
  p.data.admx.length = short_get_low_byte(length);
  memcpy(&p.data.admx.data, data, length);

  // default to bcast
  p.to.s_addr = n->state.bcast_addr.s_addr;

  if (n->state.bcast_limit == 0) {
    if ((ret = artnet_net_send(n, &p)))
      return ret;
  } else {
    int nodes;
    // find the number of ports for this uni
    SI *ips = malloc(sizeof(SI) * n->state.bcast_limit);

    if (!ips) {
      // Fallback to broadcast mode
      if ((ret = artnet_net_send(n, &p)))
        return ret;
    }

    nodes = find_nodes_from_uni(&n->node_list,
                                port->port_addr,
                                ips,
                                n->state.bcast_limit);

    if (nodes > n->state.bcast_limit) {
      // fall back to broadcast
      free(ips);
      if ((ret = artnet_net_send(n, &p))) {
        return ret;
      }
    } else {
      // unicast to the specified nodes
      int i;
      for (i =0; i < nodes; i++) {
        p.to = ips[i];
        artnet_net_send(n, &p);
      }
      free(ips);
    }
  }
  port->seq++;
  return ARTNET_EOK;
}


/*
 * Use for performance testing.
 * This allows data to be sent on any universe, not just the ones that have
 * ports configured.
 */
int artnet_raw_send_dmx(artnet_node vn,
                        uint8_t uni,
                        int16_t length,
                        const uint8_t *data) {
  node n = (node) vn;
  artnet_packet_t p;

  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type != ARTNET_RAW)
    return ARTNET_ESTATE;

  if ( length < 1 || length > ARTNET_DMX_LENGTH) {
    artnet_error("%s : Length of dmx data out of bounds (%i < 1 || %i > ARTNET_MAX_DMX)", __FUNCTION__, length);
    return ARTNET_EARG;
  }

  // set dst addr and length
  p.to.s_addr = n->state.bcast_addr.s_addr;

  p.length = sizeof(artnet_dmx_t) - (ARTNET_DMX_LENGTH - length);

  // now build packet
  memcpy( &p.data.admx.id, ARTNET_STRING, ARTNET_STRING_SIZE);
  p.data.admx.opCode = htols(ARTNET_DMX);
  p.data.admx.verH = 0;
  p.data.admx.ver = ARTNET_VERSION;
  p.data.admx.sequence = 0;
  p.data.admx.physical = 0;
  p.data.admx.universe = uni;

  // set length
  p.data.admx.lengthHi = short_get_high_byte(length);
  p.data.admx.length = short_get_low_byte(length);
  memcpy(&p.data.admx.data, data, length);

  return artnet_net_send(n, &p);
}



int artnet_send_address(artnet_node vn,
                        artnet_node_entry e,
                        const char *shortName,
                        const char *longName,
                        uint8_t inAddr[ARTNET_MAX_PORTS],
                        uint8_t outAddr[ARTNET_MAX_PORTS],
                        uint8_t subAddr, artnet_port_command_t cmd) {
  node n = (node) vn;
  artnet_packet_t p;
  node_entry_private_t *ent = find_private_entry(n,e);

  check_nullnode(vn);

  if (e == NULL || ent == NULL)
    return ARTNET_EARG;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    p.to.s_addr = ent->ip.s_addr;

    p.length = sizeof(artnet_address_t);
    p.type = ARTNET_ADDRESS;

    // now build packet, copy the number of ports from the reply recieved from this node
    memcpy( &p.data.addr.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.addr.opCode = htols(ARTNET_ADDRESS);
    p.data.addr.verH = 0;
    p.data.addr.ver = ARTNET_VERSION;
    p.data.addr.filler1 = 0;
    p.data.addr.filler2 = 0;
    strncpy((char*) &p.data.addr.shortname, shortName, ARTNET_SHORT_NAME_LENGTH);
    strncpy((char*) &p.data.addr.longname, longName, ARTNET_LONG_NAME_LENGTH);

    memcpy(&p.data.addr.swin, inAddr, ARTNET_MAX_PORTS);
    memcpy(&p.data.addr.swout, outAddr, ARTNET_MAX_PORTS);

    p.data.addr.subnet = subAddr;
    p.data.addr.swvideo = 0x00;
    p.data.addr.command = cmd;

    return artnet_net_send(n, &p);
  }
  return ARTNET_ESTATE;
}


/*
 * Sends an ArtInput packet to the specified node, this packet is used to
 * enable/disable the input ports on the remote node.
 *
 * 0x01 disable port
 * 0x00 enable port
 *
 * NOTE: should have enums here instead of uint8_t for settings
 *
 */
int artnet_send_input(artnet_node vn,
                      artnet_node_entry e,
                      uint8_t settings[ARTNET_MAX_PORTS]) {
  node n = (node) vn;
  artnet_packet_t p;
  node_entry_private_t *ent = find_private_entry(n,e);

  check_nullnode(vn);

  if (e == NULL)
    return ARTNET_EARG;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    // set dst, type and length
    p.to.s_addr = ent->ip.s_addr;

    p.length = sizeof(artnet_input_t);
    p.type = ARTNET_INPUT;

    // now build packet, copy the number of ports from the reply recieved from this node
    memcpy( &p.data.ainput.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.ainput.opCode = htols(ARTNET_INPUT);
    p.data.ainput.verH = 0;
    p.data.ainput.ver = ARTNET_VERSION;
    p.data.ainput.filler1 = 0;
    p.data.ainput.filler2 = 0;
    p.data.ainput.numbportsH = short_get_high_byte(e->numbports);
    p.data.ainput.numbports = short_get_low_byte(e->numbports);
    memcpy(&p.data.ainput.input, &settings, ARTNET_MAX_PORTS);

    return artnet_net_send(n, &p);
  }
  return ARTNET_ESTATE;
}

/*
 * Reads the latest dmx data
 * @param vn the artnet node
 * @param port_id the port to read data from
 * @param length
 * @return a pointer to the dmx data, NULL on error
 */

uint8_t *artnet_read_dmx(artnet_node vn, int port_id, int *length) {
  node n = (node) vn;

  if (n == NULL)
    return NULL;

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return NULL;
  }

  *length = n->ports.out[port_id].length;
  return &n->ports.out[port_id].data[0];
}


//--------------------------------------
// Functions to change the node state (setters)

// type : server, node, mserver, raw
int artnet_set_node_type(artnet_node vn, artnet_node_type type) {
  node n = (node) vn;
  check_nullnode(vn);

  n->state.node_type = type;
  return ARTNET_EOK;
}


/**
 * Sets the artnet subnet address for this node.
 * The subnet address has nothing to do with IP addresses). An ArtNet subnet is a grouping of 16 DMX universes
 * (ie. ports)
 *
 * The subnet address is between 0 and 15. If the supplied address is larger than 15, the
 * lower 4 bits will be used in setting the address.
 *
 * It will have no effect if the node is under network control.
 *
 * Note that changing the subnet address will cause the universe addresses of all ports to change.
 *
 * @param vn the artnet_node
 * @param subnet new subnet address
 */
int artnet_set_subnet_addr(artnet_node vn, uint8_t subnet) {
  node n = (node) vn;
  int i, ret;

  check_nullnode(vn);

  n->state.default_subnet = subnet;

  // if not under network control, and the subnet is different from the current one
  if (!n->state.subnet_net_ctl && subnet != n->state.subnet) {
    n->state.subnet = subnet;

    // redo the addresses for each port
    for (i =0; i < ARTNET_MAX_PORTS; i++) {
      n->ports.in[i].port_addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (n->ports.in[i].port_addr & LOW_NIBBLE);
      // reset dmx sequence number
      n->ports.in[i].seq = 0;

      n->ports.out[i].port_addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (n->ports.out[i].port_addr & LOW_NIBBLE);
    }

    if (n->state.mode == ARTNET_ON) {

      if ((ret = artnet_tx_build_art_poll_reply(n)))
        return ret;

      return artnet_tx_poll_reply(n,FALSE);
    }
  } else if (n->state.subnet_net_ctl ) {
    //  trying to change subnet addr while under network control
    n->state.report_code = ARTNET_RCUSERFAIL;
  }

  return ARTNET_EOK;
}


/**
 * Sets the short name of the node.
 * The string should be null terminated and a maxmium of 18 Characters will be used
 *
 * @param vn the artnet_node
 * @param name the short name of the node.
 */
int artnet_set_short_name(artnet_node vn, const char *name) {
  node n = (node) vn;
  check_nullnode(vn);

  strncpy((char *) &n->state.short_name, name, ARTNET_SHORT_NAME_LENGTH);
  n->state.short_name[ARTNET_SHORT_NAME_LENGTH-1] = 0x00;
  return artnet_tx_build_art_poll_reply(n);
}


/*
 * Sets the long name of the node.
 * The string should be null terminated and a maximium of 64 characters will be used
 *
 * @param vn the artnet_node
 * @param name the node's long name
 */
int artnet_set_long_name(artnet_node vn, const char *name) {
  node n = (node) vn;
  check_nullnode(vn);

  strncpy((char *) &n->state.long_name, name, ARTNET_LONG_NAME_LENGTH);
  n->state.long_name[ARTNET_LONG_NAME_LENGTH-1] = 0x00;
  return artnet_tx_build_art_poll_reply(n);
}


/*
 * Sets the direction and type of port
 * @param vn the artnet_node
 * @param id
 * @param direction
 * @param data
 */
int artnet_set_port_type(artnet_node vn,
                         int port_id,
                         artnet_port_settings_t settings,
                         artnet_port_data_code data) {
  node n = (node) vn;
  check_nullnode(vn);

  if (port_id < 0 || port_id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, port_id);
    return ARTNET_EARG;
  }

  n->ports.types[port_id] = settings | data;
  return ARTNET_EOK;
}


/*
 * Sets the port address of the port.
 *
 * Just to set some terms straight:
 *  - subnet address, is 4 bits, set on a per-node basis
 *  - port address, 4 bits, set on a per-port basis
 *  - universe address, 8 bits derrived from the subnet and port addresses, specific (but may
 *  not be unique) to a port.
 *
 * The upper four bits of the universe address are from the subnet address, while the lower
 * four are from the port address.
 *
 * So for example, if the subnet address of the node is 0x03, and the port address is
 * 0x02, the universe address for the port will be 0x32.
 *
 * As the port address is between 0 and 15, only the lower 4 bits of the addr argument
 * will be used.
 *
 * The operation may have no affect if the port is under network control.
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PORT
 * @param addr the new port address
 */
int artnet_set_port_addr(artnet_node vn,
                         int id,
                         artnet_port_dir_t dir,
                         uint8_t addr) {
  node n = (node) vn;
  int ret;
  int changed = 0;

  g_port_t *port;

  check_nullnode(vn);

  if (id < 0 || id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
    return ARTNET_EARG;
  }

  if (addr > 16) {
    artnet_error("%s : Attempt to set port %i to invalid address %#hhx\n", __FUNCTION__, id, addr);
    return ARTNET_EARG;
  }

  if (dir == ARTNET_INPUT_PORT) {
    port = &n->ports.in[id].port;
    changed = n->ports.in[id].port_enabled?0:1;
    n->ports.in[id].port_enabled = TRUE;
  } else if (dir == ARTNET_OUTPUT_PORT) {
    port = &n->ports.out[id].port;
    changed = n->ports.out[id].port_enabled?0:1;
    n->ports.out[id].port_enabled = TRUE;
  } else {
    artnet_error("%s : Invalid port direction\n", __FUNCTION__);
    return ARTNET_EARG;
  }

  port->default_addr = addr;

  // if not under network control and address is changing
  if (!port->net_ctl &&
      (changed || (addr & LOW_NIBBLE) != (port->addr & LOW_NIBBLE))) {
    port->addr = ((n->state.subnet & LOW_NIBBLE) << 4) | (addr & LOW_NIBBLE);

    // reset seq if input port
    if (dir == ARTNET_INPUT_PORT)
      n->ports.in[id].seq = 0;

    if (n->state.mode == ARTNET_ON) {
      if ((ret = artnet_tx_build_art_poll_reply(n)))
        return ret;

      return artnet_tx_poll_reply(n,FALSE);
    }
  } else if (port->net_ctl) {
    //  trying to change port addr while under network control
    n->state.report_code = ARTNET_RCUSERFAIL;
  }
  return ARTNET_EOK;
}


/*
 * Returns the universe address of this port
 *
 * @param vn the artnet_node
 * @param id the phyiscal port number (from 0 to ARTNET_MAX_PORTS-1 )
 * @param dir either ARTNET_INPUT_PORT or ARTNET_OUTPUT_PO
 *
 * @return the universe address, or < 0 on error
 */
int artnet_get_universe_addr(artnet_node vn, int id, artnet_port_dir_t dir) {
  node n = (node) vn;
  check_nullnode(vn);

  if (id < 0 || id >= ARTNET_MAX_PORTS) {
    artnet_error("%s : port index out of bounds (%i < 0 || %i > ARTNET_MAX_PORTS)", __FUNCTION__, id);
    return ARTNET_EARG;
  }

  if (dir == ARTNET_INPUT_PORT)
    return n->ports.in[id].port.addr;
  else if (dir == ARTNET_OUTPUT_PORT)
    return n->ports.out[id].port.addr;
  else {
    artnet_error("%s : Invalid port direction\n", __FUNCTION__);
    return ARTNET_EARG;
  }
}

int artnet_get_config(artnet_node vn, artnet_node_config_t *config) {
  int i;
  node n = (node) vn;
  check_nullnode(vn);

  strncpy(config->short_name, n->state.short_name, ARTNET_SHORT_NAME_LENGTH);
  strncpy(config->long_name, n->state.long_name, ARTNET_LONG_NAME_LENGTH);
  config->subnet = n->state.subnet;

  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    config->in_ports[i] = n->ports.in[i].port.addr & LOW_NIBBLE;
    config->out_ports[i] = n->ports.out[i].port.addr & LOW_NIBBLE;
  }

  return ARTNET_EOK;
}


/*
 * Dumps the node config to stdout.
 *
 * @param vn the artnet_node
 */
int artnet_dump_config(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  printf("#### NODE CONFIG ####\n");
  printf("Node Type: %i\n", n->state.node_type);
  printf("Short Name: %s\n", n->state.short_name);
  printf("Long Name: %s\n", n->state.long_name);
  printf("Subnet: %#02x\n", n->state.subnet);
  printf("Default Subnet: %#02x\n", n->state.default_subnet);
  printf("Net Ctl: %i\n", n->state.subnet_net_ctl);
  printf("#####################\n");

  return ARTNET_EOK;
}


/*
 * Returns the socket descriptor associated with this artnet_node.
 * libartnet currently uses two descriptors per node, one bound
 * to the network address and one bound to the subnet broadcast address
 *
 * @param vn the artnet_node
 * @param socket the index of the socket descriptor to fetch (0 or 1)
 * @return the socket descriptor
 */
int artnet_get_sd(artnet_node vn) {
  node n = (node) vn;
  check_nullnode(vn);

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  return n->sd;
}


/**
 * Sets the file descriptors in the fdset that we are interested in.
 *
 * @param vn the artnet_node
 * @param fdset pointer to the fdset to change
 * @return the maxfd+1
 */
int artnet_set_fdset(artnet_node vn, fd_set *fdset) {
  node n = (node) vn;
  check_nullnode(vn);

  if (!fdset)
    return ARTNET_EARG;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  return artnet_net_set_fdset(n, fdset);
}


/**
 * Returns the artnet_node_list.
 * The artnet_node_list holds artnet_node_entry(s) that represent the discovered
 * remote nodes on the network
 * NOTE: this function is not THREAD SAFE
 *
 * @param vn the artnet_node
 * @return the artnet_node_list
 */
artnet_node_list artnet_get_nl(artnet_node vn) {
  node n = (node) vn;

  if (!vn)
    return NULL;

  return &n->node_list;
}


/**
 * Repositions the pointer to the first entry in the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the first artnet_node_entry in the list, or NULL if the list is empty
 */
artnet_node_entry artnet_nl_first(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl)
    return NULL;

  nl->current = nl->first;
  return &nl->current->pub;
}


/**
 * Moves the pointer to the next element in the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the next artnet_node_entry, or NULL if the end of the list is reached
 */
artnet_node_entry artnet_nl_next(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl)
    return NULL;

  nl->current = nl->current->next;
  return &nl->current->pub;
}


/*
 * Returns the length of the artnet_node_list
 * NOTE: this function is not THREAD SAFE
 *
 * @param vnl the artnet_node_list
 * @return the length of the list
 */
int artnet_nl_get_length(artnet_node_list vnl) {
  node_list_t *nl = (node_list_t*) vnl;

  if (!nl)
    return 0;

  return nl->length;
}


/*
 * Return a pointer to the staticly allocated error string
 */
char *artnet_strerror() {
  return artnet_errstr;
}


//-----------------------------------------------------------------------------
// Private functions follow
//-----------------------------------------------------------------------------

int artnet_nl_update(node_list_t *nl, artnet_packet reply) {
  node_entry_private_t *entry;

  entry = find_entry_from_ip(nl, reply->from);

  if (!entry) {
    // add to list
    entry = (node_entry_private_t*) malloc(sizeof(node_entry_private_t));

    if (!entry) {
      artnet_error_malloc();
      return ARTNET_EMEM;
    }

    memset(entry, 0x00, sizeof(node_entry_private_t));

    copy_apr_to_node_entry(&entry->pub, &reply->data.ar);
    entry->ip = reply->from;
    entry->next = NULL;

    if (!nl->first) {
      nl->first = entry;
      nl->last = entry;
    } else {
      nl->last->next = entry;
      nl->last = entry;
    }
    nl->length++;
  } else {
    // update entry
    copy_apr_to_node_entry(&entry->pub, &reply->data.ar);
  }
  return ARTNET_EOK;
}


/*
 * check if this packet is in list
 */
node_entry_private_t *find_entry_from_ip(node_list_t *nl, SI ip) {
  node_entry_private_t *tmp;

  for (tmp = nl->first; tmp; tmp = tmp->next) {
    if (ip.s_addr == tmp->ip.s_addr)
      break;
  }
  return tmp;
}


/*
 * Find all nodes with a port bound to a particular universe
 * @param nl the node list
 * @param uni the universe to search for
 * @param ips store matching node ips here
 * @param size size of ips
 * @return number of nodes matched
 */
int find_nodes_from_uni(node_list_t *nl, uint8_t uni, SI *ips, int size) {
  node_entry_private_t *tmp;
  int count = 0;
  int i,j = 0;

  for (tmp = nl->first; tmp; tmp = tmp->next) {
    int added = FALSE;
    for (i =0; i < tmp->pub.numbports; i++) {
      if (tmp->pub.swout[i] == uni && ips) {
        if (j < size && !added) {
          ips[j++] = tmp->ip;
          added = TRUE;
        }
        count++;
      }
    }
  }
  return count;
}


/*
 * Add a node to the node list from an ArtPollReply msg
 */
void copy_apr_to_node_entry(artnet_node_entry e, artnet_reply_t *reply) {

  // the ip is network byte ordered
  memcpy(&e->ip, &reply->ip, 4);
  e->ver = bytes_to_short(reply->verH, reply->ver);
  e->sub = bytes_to_short(reply->subH, reply->sub);
  e->oem = bytes_to_short(reply->oemH, reply->oem);
  e->ubea = reply->ubea;
  memcpy(&e->etsaman, &reply->etsaman, 2);
  memcpy(&e->shortname, &reply->shortname,  sizeof(e->shortname));
  memcpy(&e->longname, &reply->longname, sizeof(e->longname));
  memcpy(&e->nodereport, &reply->nodereport, sizeof(e->nodereport));
  e->numbports = bytes_to_short(reply->numbportsH, reply->numbports);
  memcpy(&e->porttypes, &reply->porttypes, ARTNET_MAX_PORTS);
  memcpy(&e->goodinput, &reply->goodinput, ARTNET_MAX_PORTS);
  memcpy(&e->goodinput, &reply->goodinput, ARTNET_MAX_PORTS);
  memcpy(&e->goodoutput, &reply->goodoutput, ARTNET_MAX_PORTS);
  memcpy(&e->swin, &reply->swin, ARTNET_MAX_PORTS);
  memcpy(&e->swout, &reply->swout, ARTNET_MAX_PORTS);
  e->swvideo = reply->swvideo;
  e->swmacro = reply->swmacro;
  e->swremote = reply->swremote;
  e->style = reply->style;
  memcpy(&e->mac, &reply->mac, ARTNET_MAC_SIZE);
}

/*
 * find a node_entry in the node list
 */
node_entry_private_t *find_private_entry(node n, artnet_node_entry e) {
  node_entry_private_t *tmp;
  if (!e)
    return NULL;

  // check if this packet is in list
  for (tmp = n->node_list.first; tmp; tmp = tmp->next) {
    if (!memcmp(&e->ip, &tmp->pub.ip, 4))
      break;
  }
  return tmp;
}

/*
 * Free memory used by the iface's list
 * @param head a pointer to the head of the list
 */
static void free_ifaces(iface_t *head) {
  iface_t *ift, *ift_next;

  for (ift = head; ift != NULL; ift = ift_next) {
    ift_next = ift->next;
    free(ift);
  }
}


/*
 * Add a new interface to an interface list
 * @param head pointer to the head of the list
 * @param tail pointer to the end of the list
 * @return a new iface_t or void
 */
static iface_t *new_iface(iface_t **head, iface_t **tail) {
  iface_t *iface = (iface_t*) calloc(1, sizeof(iface_t));

  if (!iface) {
    artnet_error("%s: calloc error %s" , __FUNCTION__, strerror(errno));
    return NULL;
  }
  memset(iface, 0, sizeof(iface_t));

  if (!*head) {
    *head = *tail = iface;
  } else {
    (*tail)->next = iface;
    *tail = iface;
  }
  return iface;
}




/*
 * Check if we are interested in this interface
 * @param ifa a pointer to a ifaddr struct
 */
static void add_iface_if_needed(iface_t **head, iface_t **tail,
                                struct ifaddrs *ifa) {

  // skip down, loopback and non inet interfaces
  if (!ifa || !ifa->ifa_addr) return;
  if (!(ifa->ifa_flags & IFF_UP)) return;
  if (ifa->ifa_flags & IFF_LOOPBACK) return;
  if (ifa->ifa_addr->sa_family != AF_INET) return;

  iface_t *iface = new_iface(head, tail);
  struct sockaddr_in *sin = (struct sockaddr_in*) ifa->ifa_addr;
  iface->ip_addr.sin_addr = sin->sin_addr;
  strncpy(iface->if_name, ifa->ifa_name, IFNAME_SIZE - 1);

  if (ifa->ifa_flags & IFF_BROADCAST) {
    sin = (struct sockaddr_in *) ifa->ifa_broadaddr;
    iface->bcast_addr.sin_addr = sin->sin_addr;
  }
}


/*
 * Set if_head to point to a list of iface_t structures which represent the
 * interfaces on this machine
 * @param ift_head the address of the pointer to the head of the list
 */
static int get_ifaces(iface_t **if_head) {
  struct ifaddrs *ifa_list, *ifa_iter;
  iface_t *if_tail, *if_iter;
  struct sockaddr_ll *sll;
  char *if_name, *cptr;
  *if_head = if_tail = NULL;

  if (getifaddrs(&ifa_list) != 0) {
    artnet_error("Error getting interfaces: %s", strerror(errno));
    return ARTNET_ENET;
  }

  for (ifa_iter = ifa_list; ifa_iter; ifa_iter = ifa_iter->ifa_next)
    add_iface_if_needed(if_head, &if_tail, ifa_iter);

  // Match up the interfaces with the corrosponding AF_PACKET interface
  // to fetch the hw addresses
  //
  // TODO: Will probably not work on OS X, it should
  //      return AF_LINK -type sockaddr
  for (if_iter = *if_head; if_iter; if_iter = if_iter->next) {
    if_name = strdup(if_iter->if_name);

    // if this is an alias, get mac of real interface
    if ((cptr = strchr(if_name, ':')))
      *cptr = 0;

    // Find corresponding iface_t structure
    for (ifa_iter = ifa_list; ifa_iter; ifa_iter = ifa_iter->ifa_next) {
      if ((!ifa_iter->ifa_addr) || ifa_iter->ifa_addr->sa_family != AF_PACKET)
        continue;

      if (strncmp(if_name, ifa_iter->ifa_name, IFNAME_SIZE) == 0) {
        // Found matching hw-address
        sll = (struct sockaddr_ll*) ifa_iter->ifa_addr;
        memcpy(if_iter->hw_addr, sll->sll_addr, ARTNET_MAC_SIZE);
        break;
      }
    }
    free(if_name);
  }
  freeifaddrs(ifa_list);
  return 0;
}

/*
 * Scan for interfaces, and work out which one the user wanted to use.
 */
int artnet_net_init(node n, const char *preferred_ip) {
  iface_t *ift, *ift_head = NULL;
  struct in_addr wanted_ip;

  int found = FALSE;
  int i;
  int ret = ARTNET_EOK;

  if ((ret = get_ifaces(&ift_head)))
    goto e_return;

  if (n->state.verbose) {
    printf("#### INTERFACES FOUND ####\n");
    for (ift = ift_head; ift != NULL; ift = ift->next) {
      printf("IP: %s\n", inet_ntoa(ift->ip_addr.sin_addr));
      printf("  bcast: %s\n" , inet_ntoa(ift->bcast_addr.sin_addr));
      printf("  hwaddr: ");
      for (i = 0; i < ARTNET_MAC_SIZE; i++) {
        if (i)
          printf(":");
        printf("%02x", (uint8_t) ift->hw_addr[i]);
      }
      printf("\n");
    }
    printf("#########################\n");
  }

  if (preferred_ip) {
    // search through list of interfaces for one with the correct address
    ret = artnet_net_inet_aton(preferred_ip, &wanted_ip);
    if (ret)
      goto e_cleanup;

    for (ift = ift_head; ift != NULL; ift = ift->next) {
      if (ift->ip_addr.sin_addr.s_addr == wanted_ip.s_addr) {
        found = TRUE;
        n->state.ip_addr = ift->ip_addr.sin_addr;
        n->state.bcast_addr = ift->bcast_addr.sin_addr;
        memcpy(&n->state.hw_addr, &ift->hw_addr, ARTNET_MAC_SIZE);
        break;
      }
    }
    if (!found) {
      artnet_error("Cannot find ip %s", preferred_ip);
      ret = ARTNET_ENET;
      goto e_cleanup;
    }
  } else {
    if (ift_head) {
      // pick first address
      // copy ip address, bcast address and hardware address
      n->state.ip_addr = ift_head->ip_addr.sin_addr;
      n->state.bcast_addr = ift_head->bcast_addr.sin_addr;
      memcpy(&n->state.hw_addr, &ift_head->hw_addr, ARTNET_MAC_SIZE);
    } else {
      artnet_error("No interfaces found!");
      ret = ARTNET_ENET;
    }
  }

e_cleanup:
  free_ifaces(ift_head);
e_return :
  return ret;
}


/*
 * Start listening on the socket
 */
int artnet_net_start(node n) {
  int sock;
  struct sockaddr_in servAddr;
  int true_flag = TRUE;
  node tmp;

  // only attempt to bind if we are the group master
  if (n->peering.master == TRUE) {

    // create socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
      artnet_error("Could not create socket %s", artnet_net_last_error());
      return ARTNET_ENET;
    }

    memset(&servAddr, 0x00, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(ARTNET_PORT);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (n->state.verbose)
      printf("Binding to %s \n", inet_ntoa(servAddr.sin_addr));

    // bind sockets
    if (bind(sock, (SA *) &servAddr, sizeof(servAddr)) == -1) {
      artnet_error("Failed to bind to socket %s", artnet_net_last_error());
      artnet_net_close(sock);
      return ARTNET_ENET;
    }

    // allow bcasting
    if (setsockopt(sock,
                   SOL_SOCKET,
                   SO_BROADCAST,
                   (char*) &true_flag, // char* for win32
                   sizeof(int)) == -1) {
      artnet_error("Failed to bind to socket %s", artnet_net_last_error());
      artnet_net_close(sock);
      return ARTNET_ENET;
    }

    n->sd = sock;
    // Propagate the socket to all our peers
    for (tmp = n->peering.peer; tmp && tmp != n; tmp = tmp->peering.peer)
      tmp->sd = sock;
  }
  return ARTNET_EOK;
}


/*
 * Receive a packet.
 */
int artnet_net_recv(node n, artnet_packet p, int delay) {
  ssize_t len;
  struct sockaddr_in cliAddr;
  socklen_t cliLen = sizeof(cliAddr);
  fd_set rset;
  struct timeval tv;
  int maxfdp1 = n->sd + 1;

  FD_ZERO(&rset);
  FD_SET((unsigned int) n->sd, &rset);

  tv.tv_usec = 0;
  tv.tv_sec = delay;
  p->length = 0;

  switch (select(maxfdp1, &rset, NULL, NULL, &tv)) {
    case 0:
      // timeout
      return RECV_NO_DATA;
      break;
    case -1:
      if ( errno != EINTR) {
        artnet_error("Select error %s", artnet_net_last_error());
        return ARTNET_ENET;
      }
      return ARTNET_EOK;
      break;
    default:
      break;
  }

  // need a check here for the amount of data read
  // should prob allow an extra byte after data, and pass the size as sizeof(Data) +1
  // then check the size read and if equal to size(data)+1 we have an error
  len = recvfrom(n->sd,
                 (char*) &(p->data), // char* for win32
                 sizeof(p->data),
                 0,
                 (SA*) &cliAddr,
                 &cliLen);
  if (len < 0) {
    artnet_error("Recvfrom error %s", artnet_net_last_error());
    return ARTNET_ENET;
  }

  if (cliAddr.sin_addr.s_addr == n->state.ip_addr.s_addr ||
      ntohl(cliAddr.sin_addr.s_addr) == LOOPBACK_IP) {
    p->length = 0;
    return ARTNET_EOK;
  }

  p->length = len;
  memcpy(&(p->from), &cliAddr.sin_addr, sizeof(struct in_addr));
  // should set to in here if we need it
  return ARTNET_EOK;
}


/*
 * Send a packet.
 */
int artnet_net_send(node n, artnet_packet p) {
  struct sockaddr_in addr;
  int ret;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(ARTNET_PORT);
  addr.sin_addr = p->to;
  p->from = n->state.ip_addr;

  if (n->state.verbose)
    printf("sending to %s\n" , inet_ntoa(addr.sin_addr));

  ret = sendto(n->sd,
               (char*) &p->data, // char* required for win32
               p->length,
               0,
               (SA*) &addr,
               sizeof(addr));
  if (ret == -1) {
    artnet_error("Sendto failed: %s", artnet_net_last_error());
    n->state.report_code = ARTNET_RCUDPFAIL;
    return ARTNET_ENET;

  } else if (p->length != ret) {
    artnet_error("failed to send full datagram");
    n->state.report_code = ARTNET_RCSOCKETWR1;
    return ARTNET_ENET;
  }

  if (n->callbacks.send.fh) {
    get_type(p);
    n->callbacks.send.fh(n, p, n->callbacks.send.data);
  }
  return ARTNET_EOK;
}


/*
int artnet_net_reprogram(node n) {
  iface_t *ift_head, *ift;
  int i;

  ift_head = get_ifaces(n->sd[0]);

  for (ift = ift_head;ift != NULL; ift = ift->next ) {
    printf("IP: %s\n", inet_ntoa(ift->ip_addr.sin_addr) );
    printf("  bcast: %s\n" , inet_ntoa(ift->bcast_addr.sin_addr) );
    printf("  hwaddr: ");
      for(i = 0; i < 6; i++ ) {
        printf("%hhx:", ift->hw_addr[i] );
      }
    printf("\n");
  }

  free_ifaces(ift_head);

}*/


int artnet_net_set_fdset(node n, fd_set *fdset) {
  FD_SET((unsigned int) n->sd, fdset);
  return ARTNET_EOK;
}


/*
 * Close a socket
 */
int artnet_net_close(int sock) {
  if (close(sock)) {
    artnet_error(artnet_net_last_error());
    return ARTNET_ENET;
  }
  return ARTNET_EOK;
}

#define HAVE_INET_ATON 1
/*
 * Convert a string to an in_addr
 */
int artnet_net_inet_aton(const char *ip_address, struct in_addr *address) {
#ifdef HAVE_INET_ATON
  if (!inet_aton(ip_address, address)) {
#else
  in_addr_t *addr = (in_addr_t*) address;
  if ((*addr = inet_addr(ip_address)) == INADDR_NONE &&
      strcmp(ip_address, "255.255.255.255")) {
#endif
    artnet_error("IP conversion from %s failed", ip_address);
    return ARTNET_EARG;
  }
  return ARTNET_EOK;
}


/*
 *
 */
const char *artnet_net_last_error() {
  return strerror(errno);
}


/*
 * Checks if the callback is defined, if so call it passing the packet and
 * the user supplied data.
 * If the callbacks return a non-zero result, further processing is canceled.
 */
int check_callback(node n, artnet_packet p, callback_t callback) {
  if (callback.fh != NULL)
    return callback.fh(n, p, callback.data);

  return 0;
}


/*
 * Handle an artpoll packet
 */
int handle_poll(node n, artnet_packet p) {
  // run callback if defined
  if (check_callback(n, p, n->callbacks.poll))
    return ARTNET_EOK;

  if (n->state.node_type != ARTNET_RAW) {
    //if we're told to unicast further replies
    if (p->data.ap.ttm & TTM_REPLY_MASK) {
      n->state.reply_addr = p->from;
    } else {
      n->state.reply_addr.s_addr = n->state.bcast_addr.s_addr;
    }

    // if we are told to send updates when node conditions change
    if (p->data.ap.ttm & TTM_BEHAVIOUR_MASK) {
      n->state.send_apr_on_change = TRUE;
    } else {
      n->state.send_apr_on_change = FALSE;
    }

    return artnet_tx_poll_reply(n, TRUE);

  }
  return ARTNET_EOK;
}

/*
 * handle an art poll reply
 */
void handle_reply(node n, artnet_packet p) {
  // update the node list
  artnet_nl_update(&n->node_list, p);

  // run callback if defined
  if (check_callback(n, p, n->callbacks.reply))
    return;
}


/*
 * handle a art dmx packet
 */
void handle_dmx(node n, artnet_packet p) {
  int i, data_length;
  output_port_t *port;
  in_addr_t ipA, ipB;

  // run callback if defined
  if (check_callback(n, p, n->callbacks.dmx))
    return;

  data_length = (int) bytes_to_short(p->data.admx.lengthHi,
                                     p->data.admx.length);
  data_length = min(data_length, ARTNET_DMX_LENGTH);

  // find matching output ports
  for (i = 0; i < ARTNET_MAX_PORTS; i++) {
    // if the addr matches and this port is enabled
    if (p->data.admx.universe == n->ports.out[i].port_addr &&
        n->ports.out[i].port_enabled) {

      port = &n->ports.out[i];
      ipA = port->ipA.s_addr;
      ipB = port->ipB.s_addr;

      // ok packet matches this port
      n->ports.out[i].port_status = n->ports.out[i].port_status | PORT_STATUS_ACT_MASK;

      /**
       * 9 cases for merging depending on what the stored ips are.
       * here's the truth table
       *
       *
       * \   ipA   #           #            #             #
       *  ------   #   empty   #            #             #
       *   ipB  \  #   ( 0 )   #    p.from  #   ! p.from  #
       * ##################################################
       *           # new node  # continued  # start       #
       *  empty    # first     #  trans-    #  merge      #
       *   (0)     #   packet  #   mission  #             #
       * ##################################################
       *           #continued  #            # cont        #
       *  p.from   # trans-    # invalid!   #  merge      #
       *           #  mission  #            #             #
       * ##################################################
       *           # start     # cont       #             #
       * ! p.from  #  merge    #   merge    # discard     #
       *           #           #            #             #
       * ##################################################
       *
       * The merge exits when:
       *   o ACCancel command is received in an ArtAddress packet
       *       (this is done in handle_address )
       *   o no data is recv'ed from one source in 10 seconds
       *
       */

      check_merge_timeouts(n,i);

      if (ipA == 0 && ipB == 0) {
        // first packet recv on this port
        port->ipA.s_addr = p->from.s_addr;
        port->timeA = time(NULL);

        memcpy(&port->dataA, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA == p->from.s_addr && ipB == 0) {
        //continued transmission from the same ip (source A)

        port->timeA = time(NULL);
        memcpy(&port->dataA, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA == 0 && ipB == p->from.s_addr) {
        //continued transmission from the same ip (source B)

        port->timeB = time(NULL);
        memcpy(&port->dataB, &p->data.admx.data, data_length);
        port->length = data_length;
        memcpy(&port->data, &p->data.admx.data, data_length);
      }
      else if (ipA != p->from.s_addr  && ipB == 0) {
        // new source, start the merge
        port->ipB.s_addr = p->from.s_addr;
        port->timeB = time(NULL);
        memcpy(&port->dataB, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge, newest data is port B
        merge(n,i,data_length, port->dataB);

        // send reply if needed

      }
      else if (ipA == 0 && ipB == p->from.s_addr) {
        // new source, start the merge
        port->ipA.s_addr = p->from.s_addr;
        port->timeB = time(NULL);
        memcpy(&port->dataB, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge, newest data is portA
        merge(n,i,data_length, port->dataA);

        // send reply if needed
      }
      else if (ipA == p->from.s_addr && ipB != p->from.s_addr) {
        // continue merge
        port->timeA = time(NULL);
        memcpy(&port->dataA, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge, newest data is portA
        merge(n,i,data_length, port->dataA);

      }
      else if (ipA != p->from.s_addr && ipB == p->from.s_addr) {
        // continue merge
        port->timeB = time(NULL);
        memcpy(&port->dataB, &p->data.admx.data,data_length);
        port->length = data_length;

        // merge newest data is portB
        merge(n,i,data_length, port->dataB);

      }
      else if (ipA == p->from.s_addr && ipB == p->from.s_addr) {
//        err_warn("In handle_dmx, source matches both buffers, this shouldn't be happening!\n");

      }
      else if (ipA != p->from.s_addr && ipB != p->from.s_addr) {
//        err_warn("In handle_dmx, more than two sources, discarding data\n");

      }
      else {
//        err_warn("In handle_dmx, no cases matched, this shouldn't happen!\n");

      }

      // do the dmx callback here
      if (n->callbacks.dmx_c.fh != NULL)
        n->callbacks.dmx_c.fh(n,i, n->callbacks.dmx_c.data);
    }
  }
  return;
}


/**
 * handle art address packet.
 * This can reprogram certain nodes settings such as short/long name, port
 * addresses, subnet address etc.
 *
 */
int handle_address(node n, artnet_packet p) {
  int i, old_subnet;
  int addr[ARTNET_MAX_PORTS];
  int ret;

  if (check_callback(n, p, n->callbacks.address))
    return ARTNET_EOK;

  // servers (and raw nodes) don't respond to address packets
  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW)
    return ARTNET_EOK;

  // reprogram shortname if required
  if (p->data.addr.shortname[0] != PROGRAM_DEFAULTS &&
      p->data.addr.shortname[0] != PROGRAM_NO_CHANGE) {
    memcpy(&n->state.short_name, &p->data.addr.shortname, ARTNET_SHORT_NAME_LENGTH);
    n->state.report_code = ARTNET_RCSHNAMEOK;
  }
  // reprogram long name if required
  if (p->data.addr.longname[0] != PROGRAM_DEFAULTS &&
      p->data.addr.longname[0] != PROGRAM_NO_CHANGE) {
    memcpy(&n->state.long_name, &p->data.addr.longname, ARTNET_LONG_NAME_LENGTH);
    n->state.report_code = ARTNET_RCLONAMEOK;
  }

  // first of all store existing port addresses
  // then we can work out if they change
  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    addr[i] = n->ports.in[i].port_addr;
  }

  // program subnet
  old_subnet = p->data.addr.subnet;
  if (p->data.addr.subnet == PROGRAM_DEFAULTS) {
    // reset to defaults
    n->state.subnet = n->state.default_subnet;
    n->state.subnet_net_ctl = FALSE;

  } else if (p->data.addr.subnet & PROGRAM_CHANGE_MASK) {
    n->state.subnet = p->data.addr.subnet & ~PROGRAM_CHANGE_MASK;
    n->state.subnet_net_ctl = TRUE;
  }

  // check if subnet has actually changed
  if (old_subnet != n->state.subnet) {
    // if it does we need to change all port addresses
    for(i=0; i< ARTNET_MAX_PORTS; i++) {
      n->ports.in[i].port_addr = _make_addr(n->state.subnet, n->ports.in[i].port_addr);
      n->ports.out[i].port_addr = _make_addr(n->state.subnet, n->ports.out[i].port_addr);
    }
  }

  // program swins
  for (i =0; i < ARTNET_MAX_PORTS; i++) {
    if (p->data.addr.swin[i] == PROGRAM_NO_CHANGE)  {
      continue;
    } else if (p->data.addr.swin[i] == PROGRAM_DEFAULTS) {
      // reset to defaults
      n->ports.in[i].port_addr = _make_addr(n->state.subnet, n->ports.in[i].port_default_addr);
      n->ports.in[i].port_net_ctl = FALSE;

    } else if ( p->data.addr.swin[i] & PROGRAM_CHANGE_MASK) {
      n->ports.in[i].port_addr = _make_addr(n->state.subnet, p->data.addr.swin[i]);
      n->ports.in[i].port_net_ctl = TRUE;
    }
  }

  // program swouts
  for (i =0; i < ARTNET_MAX_PORTS; i++) {
    if (p->data.addr.swout[i] == PROGRAM_NO_CHANGE) {
      continue;
    } else if (p->data.addr.swout[i] == PROGRAM_DEFAULTS) {
      // reset to defaults
      n->ports.out[i].port_addr = _make_addr(n->state.subnet, n->ports.out[i].port_default_addr);
      n->ports.out[i].port_net_ctl = FALSE;
      n->ports.out[i].port_enabled = TRUE;
    } else if ( p->data.addr.swout[i] & PROGRAM_CHANGE_MASK) {
      n->ports.out[i].port_addr = _make_addr(n->state.subnet, p->data.addr.swout[i]);
      n->ports.in[i].port_net_ctl = TRUE;
      n->ports.out[i].port_enabled = TRUE;
    }
  }

  // reset sequence numbers if the addresses change
  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    if (addr[i] != n->ports.in[i].port_addr)
      n->ports.in[i].seq = 0;
  }

  // check command
  switch (p->data.addr.command) {
    case ARTNET_PC_CANCEL:
      // fix me
      break;
    case ARTNET_PC_RESET:
      n->ports.out[0].port_status = n->ports.out[0].port_status & ~PORT_STATUS_DMX_SIP & ~PORT_STATUS_DMX_TEST & ~PORT_STATUS_DMX_TEXT;
      // need to force a rerun of short tests here
      break;
    case ARTNET_PC_MERGE_LTP_O:
      n->ports.out[0].merge_mode = ARTNET_MERGE_LTP;
      n->ports.out[0].port_status = n->ports.out[0].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_LTP_1:
      n->ports.out[1].merge_mode = ARTNET_MERGE_LTP;
      n->ports.out[1].port_status = n->ports.out[1].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_LTP_2:
      n->ports.out[2].merge_mode = ARTNET_MERGE_LTP;
      n->ports.out[2].port_status = n->ports.out[2].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_LTP_3:
      n->ports.out[3].merge_mode = ARTNET_MERGE_LTP;
      n->ports.out[3].port_status = n->ports.out[3].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_HTP_0:
      n->ports.out[0].merge_mode = ARTNET_MERGE_HTP;
      n->ports.out[0].port_status = n->ports.out[0].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_HTP_1:
      n->ports.out[1].merge_mode = ARTNET_MERGE_HTP;
      n->ports.out[1].port_status = n->ports.out[1].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_HTP_2:
      n->ports.out[2].merge_mode = ARTNET_MERGE_HTP;
      n->ports.out[2].port_status = n->ports.out[2].port_status | PORT_STATUS_LPT_MODE;
      break;
    case ARTNET_PC_MERGE_HTP_3:
      n->ports.out[3].merge_mode = ARTNET_MERGE_HTP;
      n->ports.out[3].port_status = n->ports.out[3].port_status | PORT_STATUS_LPT_MODE;
      break;

  }

  if (n->callbacks.program_c.fh != NULL)
    n->callbacks.program_c.fh(n , n->callbacks.program_c.data);

  if ((ret = artnet_tx_build_art_poll_reply(n)))
    return ret;

  return artnet_tx_poll_reply(n, TRUE);
}


/*
 * handle art input.
 * ArtInput packets can disable input ports.
 */
int _artnet_handle_input(node n, artnet_packet p) {
  int i, ports, ret;

  if (check_callback(n, p, n->callbacks.input))
    return ARTNET_EOK;

  // servers (and raw nodes) don't respond to input packets
  if (n->state.node_type != ARTNET_NODE && n->state.node_type != ARTNET_MSRV)
    return ARTNET_EOK;

  ports = min( p->data.ainput.numbports, ARTNET_MAX_PORTS);
  for (i =0; i < ports; i++) {
    if (p->data.ainput.input[i] & PORT_DISABLE_MASK) {
      // disable
      n->ports.in[i].port_status = n->ports.in[i].port_status | PORT_STATUS_DISABLED_MASK;
    } else {
      // enable
      n->ports.in[i].port_status = n->ports.in[i].port_status & ~PORT_STATUS_DISABLED_MASK;
    }
  }

  if ((ret = artnet_tx_build_art_poll_reply(n)))
    return ret;

  return artnet_tx_poll_reply(n, TRUE);
}

/*
 * have to sort this one out.
 */
void handle_ipprog(node n, artnet_packet p) {

  if (check_callback(n, p, n->callbacks.ipprog))
    return;

  printf("in ipprog\n");
}


/*
 * The main handler for an artnet packet. calls
 * the appropriate handler function
 */
int handle(node n, artnet_packet p) {

  if (check_callback(n, p, n->callbacks.recv))
    return 0;

  switch (p->type) {
    case ARTNET_POLL:
      handle_poll(n, p);
      break;
    case ARTNET_REPLY:
      handle_reply(n,p);
      break;
    case ARTNET_DMX:
      handle_dmx(n, p);
      break;
    case ARTNET_ADDRESS:
      handle_address(n, p);
      break;
    case ARTNET_INPUT:
      _artnet_handle_input(n, p);
      break;
    case ARTNET_TODREQUEST:
      break;
    case ARTNET_TODDATA:
      break;
    case ARTNET_TODCONTROL:
      break;
    case ARTNET_RDM:
      break;
    case ARTNET_VIDEOSTEUP:
      printf("vid setup\n");
      break;
    case ARTNET_VIDEOPALETTE:
      printf("video palette\n");
      break;
    case ARTNET_VIDEODATA:
      printf("video data\n");
      break;
    case ARTNET_MACMASTER:
      printf("mac master\n");
      break;
    case ARTNET_MACSLAVE:
      printf("mac slave\n");
      break;
    case ARTNET_FIRMWAREMASTER:
      break;
    case ARTNET_FIRMWAREREPLY:
      break;
    case ARTNET_IPPROG :
      handle_ipprog(n, p);
      break;
    case ARTNET_IPREPLY:
      printf("ip reply\n");
      break;
    case ARTNET_MEDIA:
      printf("media \n");
      break;
    case ARTNET_MEDIAPATCH:
      printf("media patch\n");
      break;
    case ARTNET_MEDIACONTROLREPLY:
      printf("media control reply\n");
      break;
    default:
      n->state.report_code = ARTNET_RCPARSEFAIL;
      printf("artnet but not yet implemented!, op was %x\n", (int) p->type);
  }
  return 0;
}

/**
 * this gets the opcode from a packet
 */
int16_t get_type(artnet_packet p) {
  uint8_t *data;

  if (p->length < 10)
    return 0;
  if (!memcmp(&p->data, "Art-Net\0", 8)) {
    // not the best here, this needs to be tested on different arch
    data = (uint8_t *) &p->data;

    p->type = (data[9] << 8) + data[8];
    return p->type;
  } else {
    return 0;
  }
}


/*
 * takes a subnet and an address and creates the universe address
 */
uint8_t _make_addr(uint8_t subnet, uint8_t addr) {
  return ((subnet & LOW_NIBBLE) << 4) | (addr & LOW_NIBBLE);
}


/*
 *
 */
void check_merge_timeouts(node n, int port_id) {
  output_port_t *port;
  time_t now;
  time_t timeoutA, timeoutB;
  port = &n->ports.out[port_id];
  time(&now);
  timeoutA = now - port->timeA;
  timeoutB = now - port->timeB;

  if (timeoutA > MERGE_TIMEOUT_SECONDS) {
    // A is old, stop the merge
    port->ipA.s_addr = 0;
  }

  if (timeoutB > MERGE_TIMEOUT_SECONDS) {
    // B is old, stop the merge
    port->ipB.s_addr = 0;
  }
}


/*
 * merge the data from two sources
 */
void merge(node n, int port_id, int length, uint8_t *latest) {
  int i;
  output_port_t *port;
  port = &n->ports.out[port_id];

  if (port->merge_mode == ARTNET_MERGE_HTP) {
    for (i=0; i< length; i++)
      port->data[i] = max(port->dataA[i], port->dataB[i]);
  } else {
    memcpy(port->data, latest, length);
  }
}

/*
 * Send an art poll
 *
 * @param ip the ip address to send to
 * @param ttm the talk to me value, either ARTNET_TTM_DEFAULT,
 *   ARTNET_TTM_PRIVATE or ARTNET_TTM_AUTO
 */
int artnet_tx_poll(node n, const char *ip, artnet_ttm_value_t ttm) {
  artnet_packet_t p;
  int ret;

  if (n->state.mode != ARTNET_ON)
    return ARTNET_EACTION;

  if (n->state.node_type == ARTNET_SRV || n->state.node_type == ARTNET_RAW) {
    if (ip) {
      ret = artnet_net_inet_aton(ip, &p.to);
      if (ret)
        return ret;
    } else {
      p.to.s_addr = n->state.bcast_addr.s_addr;
    }

    memcpy(&p.data.ap.id, ARTNET_STRING, ARTNET_STRING_SIZE);
    p.data.ap.opCode = htols(ARTNET_POLL);
    p.data.ap.verH = 0;
    p.data.ap.ver = ARTNET_VERSION;
    p.data.ap.ttm = ~ttm;
    p.data.ap.pad = 0;

    p.length = sizeof(artnet_poll_t);
    return artnet_net_send(n, &p);

  } else {
    artnet_error("Not sending poll, not a server or raw device");
    return ARTNET_EACTION;
  }
}

/*
 * Send an ArtPollReply
 * @param n the node
 * @param response true if this reply is in response to a network packet
 *            false if this reply is due to the node changing it's conditions
 */
int artnet_tx_poll_reply(node n, int response) {
  artnet_packet_t reply;
  int i;

  if (!response && n->state.mode == ARTNET_ON) {
    n->state.ar_count++;
  }

  reply.to = n->state.reply_addr;
  reply.type = ARTNET_REPLY;
  reply.length = sizeof(artnet_reply_t);

  // copy from a poll reply template
  memcpy(&reply.data, &n->ar_temp, sizeof(artnet_reply_t));

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    reply.data.ar.goodinput[i] = n->ports.in[i].port_status;
    reply.data.ar.goodoutput[i] = n->ports.out[i].port_status;
  }

  snprintf((char *) &reply.data.ar.nodereport,
           sizeof(reply.data.ar.nodereport),
           "%04x [%04i] libartnet",
           n->state.report_code,
           n->state.ar_count);

  return artnet_net_send(n, &reply);
}

// this is called when the node's state changes to rebuild the
// artpollreply packet
int artnet_tx_build_art_poll_reply(node n) {
  int i;

  // shorten the amount we have to type
  artnet_reply_t *ar = &n->ar_temp;

  memset(ar, 0x00, sizeof(artnet_reply_t));

  memcpy(&ar->id, ARTNET_STRING, ARTNET_STRING_SIZE);
  ar->opCode = htols(ARTNET_REPLY);
  memcpy(&ar->ip, &n->state.ip_addr.s_addr, 4);
  ar->port = htols(ARTNET_PORT);
  ar->verH = 0;
  ar->ver = 0;
  ar->subH = 0;
  ar->sub = n->state.subnet;
  ar->oemH = n->state.oem_hi;
  ar->oem = n->state.oem_lo;
  ar->ubea = 0;
  // ar->status

//  if(n->state

  // status need to be recalc everytime
  //ar->status

  // ESTA Manufacturer ID
  // Assigned 18/4/2006
  ar->etsaman[0] = n->state.esta_hi;
  ar->etsaman[1] = n->state.esta_lo;

  memcpy(&ar->shortname, &n->state.short_name, sizeof(n->state.short_name));
  memcpy(&ar->longname, &n->state.long_name, sizeof(n->state.long_name));

  // the report is generated on every send

  // port stuff here
  ar->numbportsH = 0;

  for (i = ARTNET_MAX_PORTS; i > 0; i--) {
    if (n->ports.out[i-1].port_enabled || n->ports.in[i-1].port_enabled)
      break;
  }

  ar->numbports = i;

  for (i=0; i< ARTNET_MAX_PORTS; i++) {
    ar->porttypes[i] = n->ports.types[i];
    ar->goodinput[i] = n->ports.in[i].port_status;
    ar->goodoutput[i] = n->ports.out[i].port_status;
    ar->swin[i] = n->ports.in[i].port_addr;
    ar->swout[i] = n->ports.out[i].port_addr;
  }

  ar->swvideo  = 0;
  ar->swmacro = 0;
  ar->swremote = 0;

  // spares
  ar->sp1 = 0;
  ar->sp2 = 0;
  ar->sp3 = 0;

  // hw address
  memcpy(&ar->mac, &n->state.hw_addr, ARTNET_MAC_SIZE);

  // set style
  switch (n->state.node_type) {
    case ARTNET_SRV:
      ar->style = STSERVER;
      break;
    case ARTNET_NODE:
      ar->style = STNODE;
      break;
    case ARTNET_MSRV:
      ar->style = STMEDIA;
      break;
    // we should fix this, it'll do for now
    case ARTNET_RAW:
      ar->style = STNODE;
      break;
    default:
      artnet_error("Node type not recognised!");
      ar->style = STNODE;
      return ARTNET_ESTATE;
  }

  return ARTNET_EOK;
}
