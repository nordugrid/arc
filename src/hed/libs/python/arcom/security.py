storage_actions = ['read', 'addEntry', 'removeEntry', 'delete', 'modifyPolicy', 'modifyStates', 'modifyMetadata']
identity_type = 'http://www.nordugrid.org/schemas/policy-arc/types/tls/identity'
vomsattribute_type = 'http://www.nordugrid.org/schemas/policy-arc/types/tls/vomsattribute'
storage_action_type = 'http://www.nordugrid.org/schemas/policy-arc/types/storage/action'
request_ns = 'http://www.nordugrid.org/schemas/request-arc'
all_user = 'ALL'
anonymous_user = 'ANONYMOUS'

from arcom.logger import get_logger
log = get_logger('arcom.security')

class AuthRequest:
    
    def __init__(self, message):
        auth = message.Auth()
        import arc
        xml = auth.Export(arc.SecAttr.ARCAuth)
        subject = xml.Get('RequestItem').Get('Subject')
        try:
            self.identity = str(subject.XPathLookup('//ra:SubjectAttribute[@AttributeId="%s"]' % identity_type, arc.NS({'ra':request_ns}))[0])
        except:
            # if there is no identity in the auth object (e.g. if not using TLS)
            self.identity = anonymous_user
            identity_node = subject.NewChild('ra:SubjectAttribute',arc.NS({'ra':request_ns}))
            identity_node.Set(self.identity)
            identity_node.NewAttribute('AttributeId').Set(identity_type)
            identity_node.NewAttribute('Type').Set('string')
        self.subject = subject.GetXML()
    
    def get_request(self, action, format = 'ARCAuth'):
        if format not in ['ARCAuth']:
            raise Exception, 'Unsupported format %s' % format
        if format == 'ARCAuth':
            return '<Request xmlns="%s">\n  <RequestItem>\n%s\n%s  </RequestItem>\n</Request>' % \
                (request_ns, self.subject, '    <Action AttributeId="%s" Type="string">%s</Action>\n' % (storage_action_type, action))
            
    def get_identity(self):
        return self.identity
            
    def __str__(self): 
        return self.subject
    

            
class AuthPolicy(dict):

    def get_policy(self, format  = 'ARCAuth'):
        if format not in ['ARCAuth', 'StorageAuth']:
            raise Exception, 'Unsupported format %s' % format
        if format == 'ARCAuth':
            result = []
            for identity, actions in self.items():
                if identity == all_user:
                    subjects = ''
                elif identity.startswith('VOMS:'):
                    subjects = ('    <Subjects>\n' +
                                '      <Subject>\n' + 
                                '         <Attribute AttributeId="%s" Type="string" Function="match">/VO=%s/</Attribute>\n' % (vomsattribute_type, identity[5:]) +
                                '      </Subject>\n' +
                                '    </Subjects>\n')
                else:
                    subjects = ('    <Subjects>\n' +
                                '      <Subject>\n' + 
                                '        <Attribute AttributeId="%s" Type="string">%s</Attribute>\n' % (identity_type, identity) +
                                '      </Subject>\n' +
                                '    </Subjects>\n')
                raw_actions = [a for a in actions if a[1:] in storage_actions]
                actions = {}
                actions[True] = [action[1:] for action in raw_actions if action[0] == '+']
                actions[False] = [action[1:] for action in raw_actions if action[0] != '+']
                for permit, action_list in actions.items():
                    if action_list:
                        result.append('  <Rule Effect="%s">\n' % (permit and 'Permit' or 'Deny') +
                        '    <Description>%s is %s to %s</Description>\n' % (identity, permit and 'allowed' or 'not allowed', ', '.join(action_list)) +
                        subjects +
                        '    <Actions>\n' + 
                        ''.join(['      <Action AttributeId="%s" Type="string">%s</Action>\n' % (storage_action_type, action) for action in action_list]) +
                        '    </Actions>\n' +
                        '  </Rule>\n')
            return '<Policy xmlns="http://www.nordugrid.org/schemas/policy-arc" CombiningAlg="Deny-Overrides">\n%s</Policy>\n' % ''.join(result)            
        if format == 'StorageAuth':
            return [(identity, ' '.join([a for a in actions if a[1:] in storage_actions])) for identity, actions in self.items()]
    
    def set_policy(self, policy, format = 'StorageAuth'):
        if format != 'StorageAuth':
            raise Exception, 'Unsupported format %s' % format
        self.clear()
        if format == 'StorageAuth':
            for identity, actionstring in policy:
                self[identity] = actionstring.split()

def make_decision(policy, request):
    import arc
    loader = arc.EvaluatorLoader()
    evaluator = loader.getEvaluator('arc.evaluator')
    p = loader.getPolicy('arc.policy', arc.Source(str(policy)))
    evaluator.addPolicy(p)
    r = loader.getRequest('arc.request', arc.Source(str(request)))
    response = evaluator.evaluate(r)
    responses = response.getResponseItems()
    response_list = [responses.getItem(i).res for i in range(responses.size())]
    #print 'RESPONSE_LIST = ', response_list
    return response_list[0]
    # if response_list.count(arc.DECISION_DENY) > 0:
    #     return 'deny'
    # if response_list.count(arc.DECISION_PERMIT) > 0:
    #     return 'permit'
    # if response_list.count(arc.DECISION_NOT_APPLICABLE) > 0:
    #     return 'not_applicable'
    # return 'indeterminate'

def parse_ssl_config(cfg):
    try:
        client_ssl_node = cfg.Get(('ClientSSLConfig'))
        fromFile = str(client_ssl_node.Attribute('fromFile'))
        if fromFile:
            try:
                xml_string = file(fromFile).read()
                import arc
                client_ssl_node = arc.XMLNode(xml_string)
            except:
                log.msg()
                pass
        if client_ssl_node.Size() == 0:
            return {}
        ssl_config = {}
        ssl_config['key_file'] = str(client_ssl_node.Get('KeyPath'))
        ssl_config['cert_file'] = str(client_ssl_node.Get('CertificatePath'))
        ca_file = str(client_ssl_node.Get('CACertificatePath'))
        if ca_file:
            ssl_config['ca_file'] = ca_file
        else:
            ssl_config['ca_dir'] = str(client_ssl_node.Get('CACertificatesDir'))
        return ssl_config
    except:
        log.msg()
        return {}
