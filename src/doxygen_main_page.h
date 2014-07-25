/*! \mainpage The UCT IMS Client
 *
 * \section intro_sec Introduction
 *
 * This client is designed to be used with the Fraunhofer FOKUS Open IMS Core.
 *
 * Authors: David Waiting <david at crg.ee.uct.ac.za> and Richard Good <richard at crg.ee.uct.ac.za>
 *
 * (c) 2007 University of Cape Town, South Africa
 *
 * \section install_sec Main Features
 *
 * - AKAv1 and AKAv2 registration
 * - Subscribe to reg event
 * - Supports provisional response acknowledgements (PRACK) and preconditions
 * - Signalling follows service routes
 * - INVITE contains p-preferred-identity and p-access-network info
 * - Supports private and public user identities
 * - Pager-mode instant messaging
 * - DTMF tones via SIP INFO messages
 * - Can also be used as a normal SIP client
 * - Presence support
 * - XCAP Presence Rules Support
 *
 * \section features_sec Installation
 * \subsection req_lib Required Libraries
 *
 * - libosip2
 * - libosip2-dev
 * - libexosip2
 * - libexosip2-dev
 * - libortp2
 * - libortp2-dev
 * - libasound2
 * - libasound2-dev
 * - libgtk2-0
 * - libgtk2.0-dev
 * - libxml2
 * - libxml2-dev
 * - libcurl3
 * - libcurl3-dev
 *
 * \subsection Compile
 *
 * From the root directory
 *
 * <I>make</I>
 *
 * \subsection Run
 *
 * <I>./uctimsclient</I>
 *
 */
