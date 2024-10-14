
// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

const path = "/dev/pico";

// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

const name = 'display-pico';

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

const control_list = [
    'variables=drain valve,-,relay/water-drain-valve,2,1,0,drain valve: on,drain valve: off,turn it off,turn it on',
    'variables=input valve,-,relay/water-input-valve,2,1,0,input valve: on,input valve: off,turn it off,turn it on',
    'variables=mode,-,setting/mode,4,standby,running,protect,locked,mode: standby,mode: running,mode: protect,mode: locked,go to running,go to protect,go to locked,go to standby',
    'transmits=reboot,up to 5 minutes,control/reboot,1,reboot: pending,wait 5 minutes',
];

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
const app = {
    fatal: (message) => { console.error (message); process.exit (-1); },
    error: (message) => { console.error (message); },
    notice: (message) => { console.log (message); },
    info: (message) => { console.log (message); },
    debug: (message) => { if (options ['debug']) { console.log (message); } },
    open: (name, args = {}) => {
        if (!options_init ())
            app.fatal (`app_open: options_init failed`);
        if (!config_init (config_ini))
            app.fatal (`app_open: config_init failed with ${config_ini}`);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

var store_test = {
    "/badtuna/current/temp": 35.3,
    "/badtuna/setting/temp": 38.0,
    "/badtuna/setting/mode": "standby",
    "/badtuna/current/user/mesg": "",
    "/badtuna/current/user/warn": "0",
    "/badtuna/current/relay/water-drain-valve": '0',
    "/badtuna/current/relay/water-input-valve": '0',
}

const store = {
    set: (name, value) => {
        console.log (`SET ${name} ==> ${value}`);
        store_test [name] = value;  
    },
    get: (name) => {
        const value = store_test [name] || "";
        console.log (`GET ${name} <== ${value}`);
        return value;
    },
    get_all: () => {
        return store_test;
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

app.open (name, { path });

// -------------------------------------------------------------------------------------------------------

const display_serial = config.get ('display_pico_serial');
if (!display_serial) app.fatal ('config: no display_pico_serial!');
const display_pat = Number (config.get ('display_pico_pat')) * 1000;
if (!display_pat || display_pat < 5000) app.fatal ('config: no display_pico_pat!');
const temperature_min = Number (config.get ('temperature_min'));
if (!temperature_min || temperature_min < 26.0) app.fatal ('config: no temperature_min!');
const temperature_max = Number (config.get ('temperature_max'));
if (!temperature_max || temperature_max > 40.0) app.fatal ('config: no temperature_max!');

const mode_set = [ 'locked', 'protect', 'standby', 'running' ];

// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

const command_pat = {
    name: 'qqq',
    send: async function (intf) {
        intf.send_command (command_pat.name);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_put_conditions = {
    name: 'put-cond',
    recv: async function (intf, name, args) {
        args.split (';').forEach (arg => {
            const [name, value] = arg.trim ().split ('=');
            if (name == 'temp' && value)
                store.set ('/badtuna/current/temperature/display-pico', value);
        });
    },
    send: async function (intf) {
        const cond = {
            'temp': '/badtuna/current/temp',
            'setp': '/badtuna/setting/temp',
            'mode': '/badtuna/setting/mode',
            'mesg': '/badtuna/current/user/mesg',
            'warn': '/badtuna/current/user/warn'
        };
        const names = Object.keys (cond), args = Array ();
        for (var i = 0; i < names.length; i ++)
            args.push ([names [i], store.get (cond [names [i]])].join ('='));
        intf.send_command (command_put_conditions.name, args.join (';'));
    }
}

const command_get_conditions = {
    name: 'get-cond',
    recv: async function (intf, name, args) {
        command_put_conditions.send (intf);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_put_diagnostics = {
    name: 'put-diag',
    send: async function (intf) {
        const diag = store.get_all ();
        const args = Object.entries (diag).map (([name, value]) => `${name}=${value || ''}`).join (';').replaceAll ('/badtuna/', '').replaceAll ('current/', '');
        intf.send_command (command_put_diagnostics.name, args);
    }
}

const command_get_diagnostics = {
    name: 'get-diag',
    recv: async function (intf, name, args) {
        command_put_diagnostics.send (intf);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_set_temp = {
    name: 'set-temp',
    recv: async function (intf, name, args) {
        const temp = Number (args);
        if (temp >= temperature_min && temp <= temperature_max)
            store.set ('/badtuna/setting/temp', temp);
        command_put_conditions.send (intf);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_set_mode = {
    name: 'set-mode',
    recv: async function (intf, name, args) {
        const mode = args.trim ();
        if (mode_set.includes (mode))
            store.set ('/badtuna/setting/mode', mode);
        command_put_conditions.send (intf);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_ack_warn = {
    name: 'ack-warn',
    recv: async function (intf, name, args) {
        store.set ('/badtuna/current/user/warn', '0');
        store.set ('/badtuna/current/user/mesg', '');
        command_put_conditions.send (intf);
    }
}

// -------------------------------------------------------------------------------------------------------

const command_put_ctrl = {
    name: 'put-ctrl',
    recv: async function (intf, name, args) {
        app.notice (`control: ${args}`);
        args = args.split ('=');
        if (args.length == 2) {
            if (args [0].startsWith ('setting/')) store.set (`/badtuna/${args [0]}`, args [1]);
            else if (args [0].startsWith ('control/')) { }
            else store.set (`/badtuna/current/${args [0]}`, args [1]);
        }
        command_put_diagnostics.send (intf);
    },
    send: async function (intf) {
        control_list.forEach (control => intf.send_command (command_put_ctrl.name, control));
    }
}

// -------------------------------------------------------------------------------------------------------

const command_put_logs = {
    name: 'put-logs',
    recv: async function (intf, name, args) {
        if (args.startsWith ('note:')) app.notice (args.replace ('note: ', ''));
        else if (args.startsWith ('info:')) app.info (args.replace ('info: ', ''));
    }
}

// -------------------------------------------------------------------------------------------------------

class command_intf {
    constructor (port) {
        this.port = port;
    }
    send_command (name, args) {
        app.debug (`command-send: ${name}${args ? ' <-- ' + args : ''}`);
        this.port.write (name + (args && args.length > 0 ? ` ${args}`: '') + '\n');
    }
}

class command_recv extends require ('stream') {
    constructor (intf, commands) {
        super ();
        this.intf = intf;
        this.commands = commands;
    }
    recv_command (name, args) {
        const command = this.commands.find (command => command.name == name);
        if (command) {
            app.debug (`command-recv: ${name}${args ? ' --> ' + args : ''}`);
            if (command.recv)
                command.recv (this.intf, name, args);
            return true;
        }
        return false;
    }
    write (line) {
        if (line [0] == '#')
            app.debug (`> ${line}`);
        else {
            const name = line.split (' ') [0].toLowerCase (), args = line.substring (name.length + 1);
            if (!this.recv_command (name, args))
                app.error (`command-unknown: ${line}`);
        }
    }
}

// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

const { SerialPort, ReadlineParser } = require ('serialport');

function setup_port (options) {

    app.info (`port opening: ${options.path}`);

    const port = new SerialPort (options);

    port.on ('error', (err) => {
        app.info (`port error: ${err}`);
        process.exit (1);
    });

    port.on ('close', (err) => {
        app.info (`port closed: ${err}`);
        process.exit (err ? 1 : 0);
    });

    return port;
}

function setup_pipe (port, handler) {

    const output = new ReadlineParser ({ delimiter: '\r\n', includeDelimiter: false });
    output.pipe (handler);
    port.pipe (output);
}

// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

async function run () {

    const port = setup_port ({ path, baudRate: 115200 });

    const intf = new command_intf (port);
    const recv = new command_recv (intf, [
        command_pat,
        command_put_conditions,
        command_get_conditions,
        command_get_diagnostics,
        command_set_temp,
        command_set_mode,
        command_ack_warn,
        command_put_ctrl,
        command_put_logs
    ]);

    setup_pipe (port, recv);

    setTimeout (() => command_put_ctrl.send (intf), 5*1000);

    setInterval (function (intf) {
        command_pat.send (intf);
        try {
            const mesg = require ('fs').readFileSync ("/tmp/mesg.txt")?.toString ()?.trim ();
            if (mesg) {
                require ('fs').unlinkSync ("/tmp/mesg.txt");
                store.set ("/badtuna/current/user/mesg", mesg);
                store.set ("/badtuna/current/user/warn", 1);
            }
        } catch {}
    }, display_pat, intf);
}

run ().catch (function (err) {
    app.error (err);
    process.exit (1);
});

// -------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------

