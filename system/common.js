
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var etcd = {
    __vars: {
        host: '127.0.0.1',
        port: 2379
    }
};

function etcd_init () {

    const { Etcd3 } = require ('etcd3');

    etcd.set_time = async (name, datetime = (new Date ()).toISOString ().split ('.') [0] + 'Z') => await etcd.client.put (name).value (datetime);
    etcd.set = async (name, value) => await etcd.client.put (name).value (value);
    etcd.get = async (name) => await etcd.client.get (name);
    etcd.get_all = async () => await etcd.client.getAll ();
    etcd.watch = async (path, callback) => etcd.client.watch ().key (path).create ().then (watcher => watcher.on ('put', callback));
    etcd.client = new Etcd3 ({ hosts: `${etcd.__vars.host}:${etcd.__vars.port}` });
    return etcd.client;
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var logger = {
    __vars: {
        host: '127.0.0.1',
        port: 514,
        facility: 16
    }
};

function logger_init (name) {

    const syslog = require ('syslog');

    logger.error = (m) => logger?.client?.error (m);
    logger.info = (m) => logger?.client?.info (m);
    logger.notice = (m) => logger?.client?.notice (m);
    logger.client = syslog.createClient (logger.__vars.port, logger.__vars.host, { name: `badtuna-${name}`, facility: logger.__vars.facility } );
    return logger.client;
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var config = { }, config_ini = 'config.ini';

function config_init (file) {
    config.get = (name) => config?.__vars?.[name];
    try {
        const config_txt = require ('fs').readFileSync (file).toString ();
        if (config_txt != undefined)
            config.__vars = Object.fromEntries (config_txt.split ('\n').filter (line => !line.startsWith ('#') && line.length > 0).map (line => line.split ('=')));
    } catch { }
    return config.__vars;
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var options = { };

function options_init () {

    const getopt = require ('posix-getopt');

    var option;
    const parser = new getopt.BasicParser ('dc:', process.argv);
    while ((option = parser.getopt ()) !== undefined)
        switch (option.option) {
            case 'd': options ['debug'] = true; break;
            case 'c': config_ini = option.optarg; break;
        }

    return parser.optind ();
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

function app_fatal (message) {
    console.error (message);
    if (logger?.error) logger.error (message);
    process.exit (-1);
}
function app_error (message) {
    console.error (message);
    if (logger?.error) logger.error (message);
}
function app_notice (message) {
    console.log (message);
    if (logger?.notice) logger.notice (message);
}
function app_info (message) {
    console.log (message);
    if (logger?.info) logger.info (message);
}
function app_debug (message) {
    if (options ['debug']) {
        console.log (message);
        if (logger?.debug) logger.debug (message);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var app_signal_count = 0;

function app_signalled () {
    return app_signal_count > 0;
}

function app_open (name, args = {}) {

    if (!options_init ())
        app_fatal (`app_open: options_init failed`);

    if (!config_init (config_ini))
        app_fatal (`app_open: config_init failed with ${config_ini}`);

    if (!logger_init (name))
        app_fatal (`app_open: logger_init failed`);
    logger.info (`starting [nodejs v${process.versions.node}]: ${Object.entries (args).map (x => x.join ('=')).join (', ')}`);

    if (!etcd_init ())
        app_fatal (`app_open: etcd_init failed`);
    etcd.set_time (`/badtuna/system/component/${name}/started/time`);
    const value = etcd.get (`/badtuna/system/component/${name}/started/count`);
    etcd.set (`/badtuna/system/component/${name}/started/count`, (Number (value) || 0) + 1);

    ['SIGINT', 'SIGTERM', 'SIGHUP'].forEach (signal => process.on (signal, () => {
        console.error (`\nsignalled (${signal})\n`);
        app_signal_count ++;
    }));
}

function app_close () {
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

module.exports = { 
    store: etcd,
    config,
    app: { open: app_open, close: app_close, signalled: app_signalled, debug: app_debug, info: app_info, notice: app_notice, fatal: app_fatal },
};

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

/*
var name = 'test';
config_init ("../data/config.ini");
logger_init ('test');
etcd_init ();
console.log (config.get ('temperature_min'));
app_notice (`testing: ${(new Date ()).toISOString ().split ('.') [0] + 'Z'}`);
*/

