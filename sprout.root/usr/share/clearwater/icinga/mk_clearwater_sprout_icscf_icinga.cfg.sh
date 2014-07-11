icscf=5052
. /etc/clearwater/config
cat << EOF
define command{
        command_name    restart-sprout-icscf
        command_line    /usr/lib/nagios/plugins/clearwater-abort \$SERVICESTATE$ \$SERVICESTATETYPE$ \$SERVICEATTEMPT$ /var/run/sprout.pid 30
        }


define service{
        use                             cw-service         ; Name of service template to use
        host_name                       local_ip
        service_description             ICSCF SIP port open
        check_command                   poll_sip!$icscf
        event_handler                   restart-sprout-icscf
        }

EOF
