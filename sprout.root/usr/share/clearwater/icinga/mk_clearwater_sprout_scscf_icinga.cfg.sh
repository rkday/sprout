scscf=5054
. /etc/clearwater/config
http_port=9888
cat << EOF
define command{
        command_name    restart-sprout-scscf
        command_line    /usr/lib/nagios/plugins/clearwater-abort \$SERVICESTATE$ \$SERVICESTATETYPE$ \$SERVICEATTEMPT$ /var/run/sprout.pid 30
        }


define service{
        use                             cw-service         ; Name of service template to use
        host_name                       local_ip
        service_description             Sprout HTTP port open
        check_command                   http_ping!$http_port
        event_handler                   restart-sprout-scscf
        }

define service{
        use                             cw-service         ; Name of service template to use
        host_name                       local_ip
        service_description             SCSCF SIP port open
        check_command                   poll_sip!$scscf
        event_handler                   restart-sprout-scscf
        }

EOF
