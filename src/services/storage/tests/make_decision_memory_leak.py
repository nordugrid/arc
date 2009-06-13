# copy-paste of Bartender policy
policy = """<Policy xmlns="http://www.nordugrid.org/schemas/policy-arc" CombiningAlg="Deny-Overrides">
  <Rule Effect="Permit">
    <Description>ANONYMOUS is allowed to read, addEntry, removeEntry, delete, modifyPolicy, modifyStates, modifyMetadata</Description>
    <Subjects>
      <Subject>
        <Attribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">ANONYMOUS</Attribute>
      </Subject>
    </Subjects>
    <Actions>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">read</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">addEntry</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">removeEntry</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">delete</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">modifyPolicy</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">modifyStates</Action>
      <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">modifyMetadata</Action>
    </Actions>
  </Rule>
</Policy>
"""

# copy-paste of Bartender request
request = """<Request xmlns="http://www.nordugrid.org/schemas/request-arc">
  <RequestItem>
<ra:Subject xmlns:ra="http://www.nordugrid.org/schemas/request-arc"><ra:SubjectAttribute xmlns:ra="http://www.nordugrid.org/schemas/request-arc" Type="string" AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tcp/remoteendpoint">129.240.15.54:42100</ra:SubjectAttribute><ra:SubjectAttribute AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/tls/identity" Type="string">ANONYMOUS</ra:SubjectAttribute></ra:Subject>
    <Action AttributeId="http://www.nordugrid.org/schemas/policy-arc/types/storage/action" Type="string">read</Action>
  </RequestItem>
</Request>
"""

# leaking code starts here:

import arc
def make_decision(policy, request):
    loader = arc.EvaluatorLoader()
    evaluator = loader.getEvaluator('arc.evaluator')
    p = loader.getPolicy('arc.policy', arc.Source(str(policy)))
    evaluator.addPolicy(p)
    r = loader.getRequest('arc.request', arc.Source(str(request)))
    response = evaluator.evaluate(r)
    return 0

import time
for i in range(100):
    time.sleep(0.05)
    make_decision(policy, request)

